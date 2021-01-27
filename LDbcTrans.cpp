#include "LDbcTrans.h"
#include <QMessageBox>
#include <QTimer>
#include <QTextCodec>

// Static class data
const QString LDbcTrans::s_strProtocolName = "Dbc";
const QString LDbcTrans::s_strSwVersion = "1.0";


LDbcTrans::LDbcTrans()
{
    m_bIsDeviceReady = false;

    m_pCanDataManager = nullptr;
    m_pPath = nullptr;
    m_pDbcManagerDllWrapper = nullptr;

    m_hDbc = INVALID_DBC_HANDLE;

    m_pDataProc = new LDataProcessing(m_mapAllData, m_mapCaredData, m_msgNameMap, m_msgIdMap);
    // Connect processing slots
    connect(m_pDataProc, &LDataProcessing::sendSingleVar, this, &LDbcTrans::receiveSingleVar);


    m_pSettingsWidget = new LDbcSettingsWidget;
    connect(m_pSettingsWidget, &LDbcSettingsWidget::sendDeviceReady, this, &LDbcTrans::receiveDeviceReady);

    m_pSettingsRelier = new LDbcSettingsRelier;

    m_pToolWidget = new LDbcToolWidget;

    m_pScriptEngine = new LDbcScriptEngine(m_mapAllData, m_msgNameMap, m_pDataProc);
    connect(m_pScriptEngine, &LDbcScriptEngine::sendConsoleContents, m_pToolWidget, &LDbcToolWidget::addText2Console);
    connect(m_pScriptEngine, &LDbcScriptEngine::sendScriptContents, m_pToolWidget, &LDbcToolWidget::setText2Editor);
    connect(m_pScriptEngine, &LDbcScriptEngine::sendNewValue, this, &LDbcTrans::valueChangedByScript);
    connect(m_pToolWidget, &LDbcToolWidget::sendCurrentScript, m_pScriptEngine, &LDbcScriptEngine::saveScript);
    connect(m_pToolWidget, &LDbcToolWidget::reloadScript, m_pScriptEngine, &LDbcScriptEngine::reloadScript);

    m_pScriptThread = new QThread;
    m_pScriptEngine->moveToThread(m_pScriptThread);
    m_pScriptThread->start();

    m_pDataProcThread = new QThread;
    m_pDataProc->moveToThread(m_pDataProcThread);
    m_pDataProcThread->start();
}

LDbcTrans::~LDbcTrans()
{
    if(m_pDbcManagerDllWrapper) {
        m_pDbcManagerDllWrapper->DBC_Release(m_hDbc);
    }

    m_pScriptThread->quit();
    m_pScriptThread->wait();
    delete m_pScriptThread;

    m_pDataProcThread->quit();
    m_pDataProcThread->wait();
    delete m_pDataProcThread;

    delete m_pDataProc;
    delete m_pScriptEngine;

    //! \note   m_msgNameMap and m_msgIdMap shared the same value memory
    //!         delete either one is enough
    qDeleteAll(m_msgNameMap);

    if(m_pSettingsWidget) {
        delete m_pSettingsWidget, m_pSettingsWidget = NULL;
    }
    if(m_pSettingsRelier) {
        delete m_pSettingsRelier, m_pSettingsRelier = NULL;
    }
    if (m_pToolWidget) {
        delete m_pToolWidget, m_pToolWidget = NULL;
    }
}

void LDbcTrans::configurate(const LObixObjIter &a_rConfigIter)
{
    //! \note   m_msgNameMap and m_msgIdMap shared the same value memory
    //!         delete either one is enough
    qDeleteAll(m_msgNameMap);
    m_msgNameMap.clear();
    m_msgIdMap.clear();
    m_bErrorFlag = false;

    QString strDbcFile;
    QString strScriptFile;
    LObixObjIter configIter = a_rConfigIter;
    if ((configIter.getValue().getType() == eObj) && (configIter.getValue().getProperty("is") == "DBC Manager Plugin Config")) {
        configIter.moveToFirstChild();
    }
    else {
        m_bErrorFlag = true;
        return;
    }

    if ((configIter.getValue().getType() == eStr) && (configIter.getValue().getProperty("name") == "DBC file path")) {
        strDbcFile = configIter.getValue().getProperty("val");
    }
    else {
        m_bErrorFlag = true;
        return;
    }

    configIter.moveToNextSibling();
    if ((configIter.getValue().getType() == eStr) && (configIter.getValue().getProperty("name") == "Script file path")) {
        strScriptFile = configIter.getValue().getProperty("val");
    }
    else {
        m_bErrorFlag = true;
        return;
    }

    if(m_pPath != nullptr && m_pDbcManagerDllWrapper != nullptr) {
        FileInfo fileInfo;
        QTextCodec* pCodec = QTextCodec::codecForName("gb2312"); // To handle non-English path
        QString dbcFile = m_pPath->getPath(ePathProject, strDbcFile);
        strcpy_s(fileInfo.strFilePath, _MAX_FILE_PATH_, pCodec->fromUnicode(dbcFile));
        fileInfo.type = PROTOCOL_OTHER;
        fileInfo.merge = 1; // support load mutiple files.
        m_pDbcManagerDllWrapper->DBC_LoadFile(m_hDbc, &fileInfo);
        if(0 == m_pDbcManagerDllWrapper->DBC_GetMessageCount(m_hDbc)) {
            QMessageBox::warning(nullptr, QObject::tr("Warning"), "No message in DBC file: " + strDbcFile);
            return;
        }
        m_mapAllData.clear();
        QList<uint32_t> listIds;
        DBCMessage msg;
        if(m_pDbcManagerDllWrapper->DBC_GetFirstMessage(m_hDbc, &msg)) {
            DBCMessage *pMsg = new DBCMessage;
            *pMsg = msg;
            m_msgNameMap.insert(msg.strName, pMsg);
            m_msgIdMap.insert(msg.nID, pMsg);
            m_mapAllData.unite(parseDbcMsg(msg));
            listIds.append(msg.nID);
            while(m_pDbcManagerDllWrapper->DBC_GetNextMessage(m_hDbc, &msg)) {
                DBCMessage *pMsg = new DBCMessage;
                *pMsg = msg;
                m_msgNameMap.insert(msg.strName, pMsg);
                m_msgIdMap.insert(msg.nID, pMsg);
                m_mapAllData.unite(parseDbcMsg(msg));
                listIds.append(msg.nID);
            }
        }
        m_pDataProc->setAllMsgId(listIds);
        m_pScriptEngine->setScriptFile(m_pPath->getPath(ePathProject, strScriptFile));
    }
}

bool LDbcTrans::getDataList(LDataNameList &a_rDataNameList) const
{
    if(m_bErrorFlag) {
        return false;
    }
    else {
        a_rDataNameList = m_mapAllData.keys();
        return true;
    }
}

void LDbcTrans::resetCaredDataList(const LDataNameList &a_rCaredDataNameList)
{
    // clear the old list
    m_mapCaredData.clear();

    // build a new one
    LDataNameListIter i(a_rCaredDataNameList);
    QString strName;
    while (i.hasNext()) {
        strName = i.next();
        if(m_mapAllData.contains(strName)) {
            m_mapCaredData[strName] = m_mapAllData[strName];
        }
    }
}

void LDbcTrans::start()
{
    if(m_bIsDeviceReady) {
        m_pDataProc->startProcessing(true);
        m_pScriptEngine->startExec(true);
    }
}

void LDbcTrans::stop()
{
    m_pDataProc->startProcessing(false);
    m_pScriptEngine->startExec(false);
}

void LDbcTrans::setChannelPrefix(const QString &a_rPrefix)
{
    m_strPrefix = a_rPrefix;
}

const QString &LDbcTrans::getProtocolName() const
{
    return s_strProtocolName;
}

const QString &LDbcTrans::getSwVersion() const
{
    return s_strSwVersion;
}

QWidget *LDbcTrans::getSettingsWidget()
{
    return m_pSettingsWidget;
}

LSettingsRelier *LDbcTrans::getSettingsRelier()
{
    return m_pSettingsRelier;
}

QWidget *LDbcTrans::getToolWidget()
{
    return m_pToolWidget;
}

void LDbcTrans::setDataValue(QString a_strDataName, LDataValue a_dataValue)
{
    // remove prefix
    QString strNameWithoutPrefix = a_strDataName.right(a_strDataName.length()-1-m_strPrefix.length());

    if(m_mapAllData.contains(strNameWithoutPrefix)) {
        double value = a_dataValue.getValue();
        m_mapAllData[strNameWithoutPrefix]->setValue(value);
        // Send back the value
        LDataValueList valueList;
        a_dataValue.set(m_mapAllData[strNameWithoutPrefix]->signal.nValue); // Make sure the min max limit
        valueList.append(a_dataValue);
        emit notifySingleVar(a_strDataName, valueList);
    }

}

void LDbcTrans::setUniqueResource(const LUniqueResource &a_rUniqueResource)
{
    m_pCanDataManager = a_rUniqueResource.pUsbCan;
    m_pSettingsWidget->setCanDataManager(m_pCanDataManager);
    m_pDataProc->setCanDataManager(m_pCanDataManager);

    m_pPath = a_rUniqueResource.pPath;

    m_pDbcManagerDllWrapper = a_rUniqueResource.pDbcMgr;
    m_hDbc = m_pDbcManagerDllWrapper->DBC_Init();
#if 0
    m_pDbcManagerDllWrapper->DBC_SetSender(m_hDbc, OnSendFunc, m_pDataProc);
#endif
    m_pDataProc->setDbcManager(m_hDbc, m_pDbcManagerDllWrapper);
}

void LDbcTrans::pullAttributes()
{
    LAttributesMap mapAttri;

    QMapIterator<QString, LDbcData*> it(m_mapCaredData);
    while(it.hasNext()) {
        it.next();
        QString strName = it.key();
        // This point is in config file
        LAttributes attri;
        QTextCodec* pCodec = QTextCodec::codecForName("gb2312");
        if(pCodec) {
            attri.setUnit(pCodec->toUnicode(it.value()->signal.unit));
        }
        attri.setWritable(true);
        mapAttri.insert(m_strPrefix+"_"+strName, attri);
    }
    // Send map
    emit notifyVarAttributes(mapAttri);
}

void LDbcTrans::pullDataValue(QString a_strDataName)
{
    // remove prefix
    QString strNameWithoutPrefix = a_strDataName.right(a_strDataName.length()-1-m_strPrefix.length());

    if(m_mapAllData.contains(strNameWithoutPrefix)) {
        LDataValueList valueList;
        LDataValue value;
        value.set(m_mapAllData[strNameWithoutPrefix]->signal.nValue);
        valueList.append(value);
        emit notifySingleVar(a_strDataName, valueList);
    }

}

LDbcDataMap LDbcTrans::parseDbcMsg(const DBCMessage &a_rMsg)
{
    LDbcDataMap map;
    for(uint32_t count=0; count<a_rMsg.nSignalCount; ++count) {
        DBCSignal signal = a_rMsg.vSignals[count];
        QString strName = QString(signal.strName);
        if(!map.contains(strName)) {
            LDbcData* pData = new LDbcData;
            pData->signal = signal;
            pData->initRawValue();
            map.insert(strName, pData);
        }
    }

    return map;
}

void LDbcTrans::receiveSingleVar(QString a_strDataName, LDataValue a_dataValue)
{
    LDataValueList valueList;
    valueList.append(a_dataValue);
    emit notifySingleVar(m_strPrefix+"_"+a_strDataName, valueList);
}

void LDbcTrans::receiveDeviceReady(bool isConnected, unsigned long deviceIndex, unsigned long canIndex, unsigned long devType)
{
    m_bIsDeviceReady = isConnected;
    if(m_bIsDeviceReady) {
        LDataProcessing::Info info;
        info.devIndex = deviceIndex;
        info.chanelIndex = canIndex;
        info.devType = devType;
        m_pDataProc->setInfo(info);
    }
}

void LDbcTrans::valueChangedByScript(QString signalName, double value)
{
    LDataValueList valueList;
    LDataValue dataValue;
    dataValue.set(value);
    valueList.append(dataValue);
    emit notifySingleVar(m_strPrefix+"_"+signalName, valueList);
}
