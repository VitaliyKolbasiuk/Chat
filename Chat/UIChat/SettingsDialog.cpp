#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QDebug>
#include "Settings.h"


SettingsDialog::SettingsDialog(QWidget *parent) :
QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    ui->m_serverAddress->setText("127.0.0.1:1234");
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}

void SettingsDialog::on_buttonBox_accepted()
{
    if (ui->m_nickname->text().trimmed().size() == 0)
    {
        ui->m_errorText->setText("No nickname");
        return;
    }
    qDebug() << "OK";

    Settings settings;
    settings.generateKeys();
    settings.m_username = ui->m_nickname->text().trimmed().toStdString();
    settings.saveSettings();
    QDialog::accept();
}


void SettingsDialog::on_buttonBox_rejected()
{
    qDebug() << "NO";
    QDialog::reject();
}

