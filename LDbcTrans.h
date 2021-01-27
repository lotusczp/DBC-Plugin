#ifndef LDBCTRANS_H
#define LDBCTRANS_H

#include <QThread>
#include <QMutex>

#include "LCommon.h"
#include "LTransmission.h"
#include "LUniqueResource.h"
#include "LDbcData.h"
#include "LDataProcessing.h"
#include "LDbcSettingsWidget.h"
#include "LDbcSettingsRelier.h"
#include "LDbcToolWidget.h"
#include "LDbcScriptEngine.h"

class LDbcTrans : public LTransmission
{
    Q_OBJECT

public:
    LDbcTrans();
    ~LDbcTrans() Q_DECL_OVERRIDE;

    //! Configurate the transmission, like port, baudrate, etc...
    virtual void configurate(const LObixObjIter &a_rConfigIter) Q_DECL_OVERRIDE;

    //! return false if some error occurs when parsing the configuration tree,
    //! else return true and set the a_rWholeDataList the whole data list included in the configuration tree
    virtual bool getDataList(LDataNameList &a_rDataNameList) const Q_DECL_OVERRIDE;

    //! Clear the current data map container and fill in new data
    //! The data is user cared variables.
    virtual void resetCaredDataList(const LDataNameList &a_rCaredDataNameList) Q_DECL_OVERRIDE;

    //! Start data processing and transmission
    virtual void start() Q_DECL_OVERRIDE;

    //! Stop data processing and transmission
    virtual void stop() Q_DECL_OVERRIDE;

    //! Set the prefix of channel name to all the cared data value uploaded
    //! So that the main program can distinguish datas with the same name from different channels but using the same transmission plugin
    virtual void setChannelPrefix(const QString &a_rPrefix) Q_DECL_OVERRIDE;

    //! Get the protocol name
    virtual const QString &getProtocolName() const Q_DECL_OVERRIDE;

    //! Get the SW version number
    virtual const QString &getSwVersion() const Q_DECL_OVERRIDE;

    //! widget for configuration of this plugin
    virtual QWidget* getSettingsWidget() Q_DECL_OVERRIDE;

    //! settings relier of this plugin
    virtual LSettingsRelier* getSettingsRelier() Q_DECL_OVERRIDE;

    //! tool widget of this plugin
    virtual QWidget* getToolWidget() Q_DECL_OVERRIDE;

    //! set value by lobster main program
    virtual void setDataValue(QString a_strDataName, LDataValue a_dataValue) Q_DECL_OVERRIDE;

    //! set the unique resource access interface
    virtual void setUniqueResource(const LUniqueResource& a_rUniqueResource) Q_DECL_OVERRIDE;

    //! Force to pull attributes from this plugin
    virtual void pullAttributes() Q_DECL_OVERRIDE;

    //! Force to pull value of a certain point from this plugin
    virtual void pullDataValue(QString a_strDataName) Q_DECL_OVERRIDE;

signals:

protected:
    //!
    LDbcDataMap parseDbcMsg(const DBCMessage& a_rMsg);

    //! Receive variables from data processing
    void receiveSingleVar(QString a_strDataName, LDataValue a_dataValue);

    //! Receive device connected
    void receiveDeviceReady(bool isConnected, unsigned long deviceIndex, unsigned long canIndex, unsigned long devType);

    void valueChangedByScript(QString signalName, double value);


protected:  // Protected const class data
    static const QString        s_strProtocolName;      //!< transmission name
    static const QString        s_strSwVersion;         //!< transmission plugin version number

    bool                        m_bErrorFlag;           //!< whether any wrong while parsing the config
    QString                     m_strPrefix;            //!< channel prefix

    QMutex                      m_mutex;                //!< mutex for m_dataProcThread and main thread
    LDataProcessing             *m_pDataProc;           //!< Data processing slots object

    bool                        m_bErrorReported;       //!< Flag of whether the error is reported

    LCanDataManager             *m_pCanDataManager;
    LPath                       *m_pPath;
    LDbcManagerDllWrapper       *m_pDbcManagerDllWrapper;

    DBCHandle                   m_hDbc;

    int                         m_iCanId;
    bool                        m_bIsDeviceReady;

    LDbcDataMap                 m_mapAllData;           //!< all the data information in the config tree
    LDbcDataMap                 m_mapCaredData;         //!< Map of user interested data source

    LDbcSettingsWidget          *m_pSettingsWidget;     //!< settings widget
    LDbcSettingsRelier          *m_pSettingsRelier;     //!< settings relier
    LDbcToolWidget              *m_pToolWidget;         //!< tool widget

    LDbcScriptEngine            *m_pScriptEngine;

    LDbcMsgNameMap              m_msgNameMap;
    LDbcMsgIdMap                m_msgIdMap;

    QThread                     *m_pScriptThread;
    QThread                     *m_pDataProcThread;
};

#endif // LDBCTRANS_H
