#include "LDbcScriptConsole.h"
#include <QScrollBar>

#define REFRESH_TIME_ms   500

LDbcScriptConsole::LDbcScriptConsole(QWidget *parent) :
    QPlainTextEdit(parent)
{
    m_pTimer = new QTimer(this);
    m_pTimer->setInterval(REFRESH_TIME_ms);
    m_pTimer->setSingleShot(true);
    connect(m_pTimer, &QTimer::timeout, this, &LDbcScriptConsole::refresh);
    m_pTimer->start();
}

LDbcScriptConsole::~LDbcScriptConsole()
{
    delete m_pTimer;
}

void LDbcScriptConsole::addText(const QString &a_rstrText)
{
    m_strText += a_rstrText;
    if(!m_pTimer->isActive()) {
        m_pTimer->start();
    }
}

void LDbcScriptConsole::clearText()
{
    m_strText.clear();
    this->clear();
}

void LDbcScriptConsole::refresh()
{
    this->appendPlainText(m_strText);
    m_strText.clear();
}


