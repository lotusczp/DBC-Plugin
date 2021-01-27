#ifndef LDBCTOOLWIDGET_H
#define LDBCTOOLWIDGET_H

#include <QWidget>

namespace Ui {
class LDbcToolWidget;
}

class LDbcToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LDbcToolWidget(QWidget *parent = nullptr);
    ~LDbcToolWidget();

signals:
    void sendCurrentScript(QString script);

    void reloadScript();

public slots:
    void addText2Console(QString str);

    void setText2Editor(QString str);

private slots:
    void on_saveButton_clicked();

    void on_refreshButton_clicked();

    void on_clearConsoleButton_clicked();

private:
    Ui::LDbcToolWidget *ui;
};

#endif // LDBCTOOLWIDGET_H
