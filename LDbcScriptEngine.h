#ifndef LDBCSCRIPTENGINE_H
#define LDBCSCRIPTENGINE_H

#include <QObject>
#include <QJSEngine>
#include <QList>
#include <QTimer>
#include "LDbcManagerDllWrapper.h"
#include "LDbcData.h"
#include "LDataProcessing.h"

class LConsoleAdapter : public QObject
{
    Q_OBJECT

public:
    explicit LConsoleAdapter(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~LConsoleAdapter() {}

signals:
    void sendContents(QString str);

public slots:
    void log(QJSValue str)
    {
        emit sendContents(str.toString()+"\n");
    }
};

class LDbcMgrAdapter : public QObject
{
    Q_OBJECT
public:
    LDbcMgrAdapter(LDbcDataMap &a_rAllDataMap,
                   LDbcMsgNameMap& a_rMsgMap,
                   QJSEngine &a_rJSEngine,
                   LDataProcessing &a_rDataProc) :
        m_mapAllData(a_rAllDataMap),
        m_rMsgMap(a_rMsgMap),
        m_rJSEngine(a_rJSEngine),
        m_rDataProc(a_rDataProc)
    {}
    virtual ~LDbcMgrAdapter() {}

signals:
    void sendNewValue(QString signalName, double value);

public slots:
    QJSValue getSigValue(QJSValue a_signalName)
    {
        QString strSignalName = a_signalName.toString();
        if(m_mapAllData.contains(strSignalName)) {
            return m_mapAllData[strSignalName]->signal.nValue;
        }
        return QJSValue();
    }

    QJSValue getMsgBytes(QJSValue a_msgName)
    {
        QString strMsgName = a_msgName.toString();
        if(m_rMsgMap.contains(strMsgName)) {
            m_rDataProc.updateMsgContent(strMsgName);
            QList<quint8> bytes = m_rDataProc.getMsgBytes(strMsgName);
            QJSValue byteArray = m_rJSEngine.newArray(8);
            for(quint32 i=0; i<8; ++i) {
                byteArray.setProperty(i, bytes[i]);
            }
            return byteArray;
        }
        return QJSValue();
    }

    void setSigValue(QJSValue a_signalName, QJSValue a_value)
    {
        QString strSignalName = a_signalName.toString();
        if(m_mapAllData.contains(strSignalName)) {
            double value = a_value.toNumber();
            if(m_mapAllData[strSignalName]->setValue(value)) {
                // Value changed
                emit sendNewValue(strSignalName, m_mapAllData[strSignalName]->signal.nValue);
            }
        }
    }

private:
    LDbcDataMap             &m_mapAllData;
    LDbcMsgNameMap          &m_rMsgMap;
    QJSEngine               &m_rJSEngine;
    LDataProcessing         &m_rDataProc;
};

class LDbcScriptEngine : public QObject
{
    Q_OBJECT
public:
    LDbcScriptEngine(LDbcDataMap& a_rAllDataMap,
                     LDbcMsgNameMap& a_rMsgMap,
                     LDataProcessing* a_pDataProc);
    virtual ~LDbcScriptEngine();

    void setScriptFile(const QString &a_rstrFile);

    void reloadScript();

    void saveScript(QString script);

    void startExec(bool start);

signals:
    void sendConsoleContents(QString str);

    void sendScriptContents(QString str);

    void sendNewValue(QString signalName, double value);

    void startTimer();

    void stopTimer();

protected slots:
    void processSendMsg();

private:
    LDbcDataMap             &m_mapAllData;
    LDataProcessing         *m_pDataProc;

    LConsoleAdapter         *m_pConsole;
    LDbcMgrAdapter          *m_pDbcMgrAdapter;

    QJSEngine               *m_pJSEngine;
    QMap<QString, QJSValue> m_mapMsgScript;
    QList<QTimer*>          m_listSendTimer;
    QString                 m_strScriptFile;
};

#endif // LDBCSCRIPTENGINE_H
