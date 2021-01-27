#ifndef LDBCSETTINGSRELIER_H
#define LDBCSETTINGSRELIER_H

#include <QObject>
#include "LSettingsRelier.h"

class LDbcSettingsRelier : public QObject, public LSettingsRelier
{
    Q_OBJECT
public:
    explicit LDbcSettingsRelier(QObject *parent = nullptr);
    virtual ~LDbcSettingsRelier() Q_DECL_OVERRIDE {}

    virtual void parseFromSetting(LObixTreeIter a_obixIter) Q_DECL_OVERRIDE;

    virtual void convertToSetting() Q_DECL_OVERRIDE;

public slots:
    void receiveServerInfo(QString a_strServer, QString a_strPort);

signals:
    void sendServerInfo(QString a_strServer, QString a_strPort);

private:
};

#endif // LDBCSETTINGSRELIER_H
