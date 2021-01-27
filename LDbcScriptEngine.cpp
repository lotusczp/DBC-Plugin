#include "LDbcScriptEngine.h"
#include <QFile>
#include <QTextStream>
#include <QJSValueIterator>

LDbcScriptEngine::LDbcScriptEngine(LDbcDataMap& a_rAllDataMap, LDbcMsgNameMap &a_rMsgMap, LDataProcessing *a_pDataProc) :
    m_mapAllData(a_rAllDataMap),
    m_pDataProc(a_pDataProc)
{
    m_pJSEngine = new QJSEngine;

    m_pConsole = new LConsoleAdapter;
    connect(m_pConsole, &LConsoleAdapter::sendContents, this, &LDbcScriptEngine::sendConsoleContents);
    QJSValue consoleObj =  m_pJSEngine->newQObject(m_pConsole);
    m_pJSEngine->globalObject().setProperty("console", consoleObj);

    m_pDbcMgrAdapter = new LDbcMgrAdapter(a_rAllDataMap, a_rMsgMap, *m_pJSEngine, *m_pDataProc);
    connect(m_pDbcMgrAdapter, &LDbcMgrAdapter::sendNewValue, this, &LDbcScriptEngine::sendNewValue);
    QJSValue jsDbcMgrAdapter =  m_pJSEngine->newQObject(m_pDbcMgrAdapter);
    m_pJSEngine->globalObject().setProperty("lbDbcMgr", jsDbcMgrAdapter);
}

LDbcScriptEngine::~LDbcScriptEngine()
{
    qDeleteAll(m_listSendTimer);

    delete m_pConsole;
    delete m_pDbcMgrAdapter;
    delete m_pJSEngine;
}

void LDbcScriptEngine::setScriptFile(const QString &a_rstrFile)
{
    m_strScriptFile = a_rstrFile;

    reloadScript();
}

void LDbcScriptEngine::reloadScript()
{
    m_mapMsgScript.clear();
    qDeleteAll(m_listSendTimer);
    m_listSendTimer.clear();

    QFile scriptFile(m_strScriptFile);
    if (!scriptFile.open(QIODevice::ReadOnly)) return;
    QTextStream stream(&scriptFile);
    QString contents = stream.readAll();
    scriptFile.close();
    emit sendScriptContents(contents);
    QJSValue result = m_pJSEngine->evaluate(contents, m_strScriptFile);
    if(result.isError()) {
        emit sendConsoleContents("Uncaught exception at line " +
                                 QString::number(result.property("lineNumber").toInt())+" : \n"+
                                 result.toString()+"\n");
    }
    else {
        QJSValueIterator it(m_pJSEngine->globalObject().property("DBCMessage"));
        while (it.hasNext()) {
            it.next();
            m_mapMsgScript.insert(it.name(), it.value());
            if(it.value().hasProperty("cycleTime_ms") && it.value().property("cycleTime_ms").toInt()!=0) {
                QTimer* pTimer = new QTimer;
                pTimer->setObjectName(it.name());
                pTimer->setInterval(it.value().property("cycleTime_ms").toInt());
                connect(pTimer, &QTimer::timeout, this, &LDbcScriptEngine::processSendMsg);
                connect(this, SIGNAL(startTimer()), pTimer, SLOT(start()));
                connect(this, &LDbcScriptEngine::stopTimer, pTimer, &QTimer::stop);
                m_listSendTimer.append(pTimer);
            }
        }
    }
}

void LDbcScriptEngine::saveScript(QString script)
{
    QFile file(m_strScriptFile);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << script;
    out.flush();
    file.close();

    reloadScript();
}

void LDbcScriptEngine::startExec(bool start)
{
    if(start) {
        emit startTimer();
    }
    else {
        emit stopTimer();
    }
}

void LDbcScriptEngine::processSendMsg()
{
    QTimer *pTimer = qobject_cast<QTimer *>(sender());
    if(pTimer) {
        QString strMsgName = pTimer->objectName();
        // Run the "OnSend" function
        if(m_mapMsgScript.contains(strMsgName)) {
            QJSValue retval = m_mapMsgScript[strMsgName].property("OnSend").call();
            if(retval.isError()) {
                emit sendConsoleContents(retval.toString());
            }
        }
        if(m_pDataProc) {
            // Update the message
            m_pDataProc->updateMsgContent(strMsgName);
            // Send this message
            m_pDataProc->sendDbcMsg(strMsgName);
        }

    }
}
