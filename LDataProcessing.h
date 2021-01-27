#ifndef LDATAPROCESSING_H
#define LDATAPROCESSING_H

#include <QObject>
#include "LCommon.h"
#include "LUniqueResource.h"
#include "LDbcData.h"

class FrameHandler : public QObject, public LCanDataReceiver
{
    Q_OBJECT
public:
    FrameHandler(unsigned int a_uiCanId) : LCanDataReceiver(a_uiCanId){}
    void receiveMsgCallback(const VCI_CAN_OBJ& a_rMsg);
signals:
    void sendCanFrame(CanObj a_canObj);
};

class LDataProcessing : public QObject
{
    Q_OBJECT

public:
    struct Info{
        unsigned devIndex;
        unsigned chanelIndex;
        unsigned devType;
    };

public:
    LDataProcessing(LDbcDataMap& a_rAllDataMap,
                    LDbcDataMap& a_rCaredDataMap,
                    LDbcMsgNameMap& a_rMsgMap,
                    LDbcMsgIdMap& a_msgIdMap);

    virtual ~LDataProcessing();

    void setCanDataManager(LCanDataManager *a_pCanDataManager);

    void setDbcManager(DBCHandle a_hDbcm, LDbcManagerDllWrapper *a_pDbcManagerDllWrapper);

    void setInfo(const LDataProcessing::Info &a_rInfo);

    void startProcessing(bool a_bStart);

    bool sendCanObj(CanObj &a_rCanObj);

    bool updateMsgContent(const QString &a_rstrMsgName);

    bool sendDbcMsg(const QString &a_rstrMsgName);

    void setAllMsgId(QList<uint32_t> listIds);

    QList<quint8> getMsgBytes(const QString &a_rstrMsgName) const;

signals:
    void sendSingleVar(QString a_strDataName, LDataValue a_dataValue);

protected slots:
    void handleCanFrame(CanObj a_canObj);

protected:
    LDbcDataMap             &m_mapAllData;          //!< All the data information in the config tree
    LDbcDataMap             &m_mapCaredData;        //!< Map of user interested data source
    LDbcMsgNameMap          &m_mapMsg;
    LDbcMsgIdMap            &m_msgIdMap;

    LCanDataManager         *m_pCanDataManager;
    LDbcManagerDllWrapper   *m_pDbcManagerDllWrapper;
    Info                    m_info;
    QList<FrameHandler*>    m_listFrameHandler;
    DBCHandle               m_hDbc;
    bool                    m_bIsStarted;
};

#endif // LDATAPROCESSING_H
