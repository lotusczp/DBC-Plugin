#ifndef LDBCSETTINGSWIDGET_H
#define LDBCSETTINGSWIDGET_H

#include <QWidget>
#include "LUniqueResource.h"

namespace Ui {
class LDbcSettingsWidget;
}

class LDbcSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LDbcSettingsWidget(QWidget *parent = nullptr);
    ~LDbcSettingsWidget();

    void setCanDataManager(LCanDataManager *a_pCanDataManager);

signals:
    void sendDeviceReady(bool isReady, unsigned long deviceIndex, unsigned long canIndex, unsigned long devType);

private slots:
    void on_buttonConnect_clicked();

    void on_buttonDisconnect_clicked();

private:
    void deviceConnected();

private:
    Ui::LDbcSettingsWidget *ui;
    LCanDataManager         *m_pCanDataManager;
};

#endif // LDBCSETTINGSWIDGET_H
