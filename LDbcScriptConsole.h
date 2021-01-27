#ifndef LDBCSCRIPTCONSOLE_H
#define LDBCSCRIPTCONSOLE_H

#include <QPlainTextEdit>
#include <QTimer>

class LDbcScriptConsole : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit LDbcScriptConsole(QWidget *parent = nullptr);
    virtual ~LDbcScriptConsole();

    void addText(const QString &a_rstrText);

    void clearText();

private slots:
    void refresh();

private:
    QString m_strText;
    QTimer *m_pTimer;
};

#endif // LDBCSCRIPTCONSOLE_H
