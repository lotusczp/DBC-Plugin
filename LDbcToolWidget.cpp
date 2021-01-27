#include "LDbcToolWidget.h"
#include "ui_LDbcToolWidget.h"

LDbcToolWidget::LDbcToolWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LDbcToolWidget)
{
    ui->setupUi(this);

    QList<int> sizeList;
    sizeList.append(400);
    sizeList.append(200);
    ui->splitter->setSizes(sizeList);

    ui->console->setReadOnly(true);
}

LDbcToolWidget::~LDbcToolWidget()
{
    delete ui;
}

void LDbcToolWidget::addText2Console(QString str)
{
    ui->console->addText(str);
}

void LDbcToolWidget::setText2Editor(QString str)
{
    ui->codeEditor->setPlainText(str);
}

void LDbcToolWidget::on_saveButton_clicked()
{
    emit sendCurrentScript(ui->codeEditor->toPlainText());
}

void LDbcToolWidget::on_refreshButton_clicked()
{
    emit reloadScript();
}

void LDbcToolWidget::on_clearConsoleButton_clicked()
{
    ui->console->clearText();
}
