#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QDebug>
#include "Settings.h"


SettingsDialog::SettingsDialog(ChatClient& chatClient, bool isNewUser, QWidget *parent) :
QDialog(parent), m_chatClient(chatClient), m_isNewUser(isNewUser), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);

    connect(ui->m_saveBtn, &QPushButton::released, this, &SettingsDialog::onSaveBtnReleased);
    connect(ui->m_cancelBtn, &QPushButton::released, this, &SettingsDialog::onCancelBtnReleased);
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}

void SettingsDialog::onSaveBtnReleased()
{
    Settings settings;

    std::string newUserName = ui->m_username->text().trimmed().toStdString();
    if (newUserName.size() == 0)
    {
        return;
    }

    if (!m_isNewUser.value())
    {
        settings.loadSettings();
        if (settings.m_username == newUserName)
        {
            QDialog::accept();
        }

        m_chatClient.changeUsername(newUserName);
    }
    else
    {
        settings.generateKeys();
    }
    m_isNewUser.reset();

    settings.m_username = newUserName;
    settings.saveSettings();
    QDialog::accept();
}


void SettingsDialog::onCancelBtnReleased()
{
    QDialog::reject();
}

