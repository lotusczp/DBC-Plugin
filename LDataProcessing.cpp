#include "LDataProcessing.h"

static uint64_t parseSignalRawBytes(const uint8_t *data, int startBit, int sigSize, bool littleEndian, bool isSigned);

static void buildMsgBytesWithSignal(uint8_t *data, uint64_t signalRawValue, int startBit, int sigSize, bool littleEndian);


void FrameHandler::receiveMsgCallback(const VCI_CAN_OBJ &a_rMsg)
{
    emit sendCanFrame(a_rMsg);
}

LDataProcessing::LDataProcessing(LDbcDataMap &a_rAllDataMap, LDbcDataMap &a_rCaredDataMap, LDbcMsgNameMap &a_rMsgMap, LDbcMsgIdMap &a_msgIdMap) :
    m_mapAllData(a_rAllDataMap),
    m_mapCaredData(a_rCaredDataMap),
    m_mapMsg(a_rMsgMap),
    m_msgIdMap(a_msgIdMap)
{
    m_pCanDataManager = nullptr;
    m_pDbcManagerDllWrapper = nullptr;

    m_hDbc = INVALID_DBC_HANDLE;
    m_bIsStarted = false;

    qRegisterMetaType<CanObj>("CanObj");
}

LDataProcessing::~LDataProcessing()
{
    qDeleteAll(m_listFrameHandler);
    m_listFrameHandler.clear();
}

void LDataProcessing::setCanDataManager(LCanDataManager *a_pCanDataManager)
{
    m_pCanDataManager = a_pCanDataManager;
}

void LDataProcessing::setDbcManager(DBCHandle a_hDbcm, LDbcManagerDllWrapper *a_pDbcManagerDllWrapper)
{
    m_hDbc = a_hDbcm;
    m_pDbcManagerDllWrapper = a_pDbcManagerDllWrapper;
}

void LDataProcessing::startProcessing(bool a_bStart)
{
    m_bIsStarted = a_bStart;
}

void LDataProcessing::setInfo(const LDataProcessing::Info &a_rInfo)
{
    m_info = a_rInfo;

    if(m_pCanDataManager) {
        foreach (FrameHandler* pFrameHandler, m_listFrameHandler) {
            m_pCanDataManager->registerReceiver(pFrameHandler, m_info.devIndex, m_info.chanelIndex, m_info.devType);
        }
    }
}

bool LDataProcessing::sendCanObj(CanObj &a_rCanObj)
{
    if(m_pCanDataManager) {
        return (m_pCanDataManager->transmit(&a_rCanObj, 1, m_info.devIndex, m_info.chanelIndex, m_info.devType)==STATUS_OK);
    }
    return false;
}

bool LDataProcessing::updateMsgContent(const QString &a_rstrMsgName)
{
    if(m_mapMsg.contains(a_rstrMsgName)) {
        for(uint32_t count=0; count<m_mapMsg[a_rstrMsgName]->nSignalCount; ++count) {
            DBCSignal signal = m_mapMsg[a_rstrMsgName]->vSignals[count];
            if(m_mapAllData.contains(signal.strName)) { // Must contains, but we never know
                // Update the signal
                m_mapMsg[a_rstrMsgName]->vSignals[count] = m_mapAllData[signal.strName]->signal;
            }
        }
        return true;
    }
    return false;
}

bool LDataProcessing::sendDbcMsg(const QString &a_rstrMsgName)
{
    if(m_mapMsg.contains(a_rstrMsgName)) {
        uint8_t bytes[8] = {0, }; // Must set all zero first
        for(uint32_t count=0; count<m_mapMsg[a_rstrMsgName]->nSignalCount; ++count) {
            DBCSignal *pSignal = &m_mapMsg[a_rstrMsgName]->vSignals[count];
            buildMsgBytesWithSignal(bytes, pSignal->nRawValue, pSignal->nStartBit, pSignal->nLen, !pSignal->is_motorola);
        }
        CanObj canObj;
        canObj.ID = m_mapMsg[a_rstrMsgName]->nID;
        canObj.DataLen = 8;
        canObj.ExternFlag = m_mapMsg[a_rstrMsgName]->nExtend;
        canObj.SendType = 1; // One shot, no retry
        canObj.RemoteFlag = 0;
        for(int i=0; i<8; ++i) {
            canObj.Data[i] = bytes[i];
        }
        return sendCanObj(canObj);
    }

    return false;
}

void LDataProcessing::setAllMsgId(QList<uint32_t> listIds)
{
    qDeleteAll(m_listFrameHandler);
    m_listFrameHandler.clear();
    foreach (uint32_t id, listIds) {
        FrameHandler *pFrameHandler = new FrameHandler(id);
        connect(pFrameHandler, &FrameHandler::sendCanFrame, this, &LDataProcessing::handleCanFrame);
        m_listFrameHandler.append(pFrameHandler);
    }
}

QList<quint8> LDataProcessing::getMsgBytes(const QString &a_rstrMsgName) const
{
    QList<quint8> byteArray;
    for(int i=0; i<8; ++i) {
        byteArray.append(0);
    }
    quint8 data[8] = {0, }; // Must set all zero first
    if(m_mapMsg.contains(a_rstrMsgName)) {
        for(uint32_t count=0; count<m_mapMsg[a_rstrMsgName]->nSignalCount; ++count) {
            DBCSignal *pSignal = &m_mapMsg[a_rstrMsgName]->vSignals[count];
            buildMsgBytesWithSignal(data, pSignal->nRawValue, pSignal->nStartBit, pSignal->nLen, !pSignal->is_motorola);
        }
        for(int i=0; i<8; ++i) {
            byteArray[i] = data[i];
        }
    }
    return byteArray;
}

void LDataProcessing::handleCanFrame(CanObj a_canObj)
{
    if(m_bIsStarted) {
        if(m_msgIdMap.contains(a_canObj.ID)) {
            for(uint32_t count=0; count<m_msgIdMap[a_canObj.ID]->nSignalCount; ++count) {
                DBCSignal *pSignal = &m_msgIdMap[a_canObj.ID]->vSignals[count];
                uint64_t rawValue = parseSignalRawBytes(a_canObj.Data, pSignal->nStartBit, pSignal->nLen, !pSignal->is_motorola, pSignal->is_signed);
                if(m_mapCaredData.contains(pSignal->strName)) {
                    m_mapCaredData[pSignal->strName]->setRawValue(rawValue);
                    LDataValue dataValue;
                    dataValue.set(m_mapCaredData[pSignal->strName]->signal.nValue);
                    emit sendSingleVar(pSignal->strName, dataValue);
                }
            }
        }
    }
}

uint64_t parseSignalRawBytes(const uint8_t *data, int startBit, int sigSize, bool littleEndian, bool isSigned)
{
    uint64_t result = 0;
    int bit;

    if (littleEndian) {
        bit = startBit;
        for (int bitpos = 0; bitpos < sigSize; bitpos++) {
            if (data[bit / 8] & (1 << (bit % 8)))
                result += (1ULL << bitpos);
            bit++;
        }
    }
    else {//motorola / big endian mode
        bit = startBit;
        for (int bitpos = 0; bitpos < sigSize; bitpos++) {
            if (data[bit / 8] & (1 << (bit % 8)))
                result += (1ULL << (sigSize - bitpos - 1));

            if ((bit % 8) == 0)
                bit += 15;
            else bit--;

        }
    }

    if (isSigned) {
        int64_t mask = (1ULL << (sigSize - 1));
        if ((result & mask) == mask) {//is the highest bit possible for this signal size set?
            uint64_t signedMask = ~((1ULL << sigSize) - 1);
            result = (-1LL & signedMask) | result;
        }
    }

    return result;

};

void buildMsgBytesWithSignal(uint8_t *data, uint64_t signalRawValue, int startBit, int sigSize, bool littleEndian)
{
    int bit;

    if(littleEndian) {
        bit = startBit;
        for (int bitpos = 0; bitpos < sigSize; bitpos++) {
            if (signalRawValue & (1 << bitpos))
                data[bit/8] += (1ULL << (bit%8));
            bit++;
        }
    }
    else {//motorola / big endian mode
        bit = startBit;
        for (int bitpos = 0; bitpos < sigSize; bitpos++) {
            if (signalRawValue & (1 << (sigSize-bitpos-1)))
                data[bit/8] += (1ULL << (bit%8));

            if ((bit % 8) == 0)
                bit += 15;
            else bit--;

        }
    }
}
