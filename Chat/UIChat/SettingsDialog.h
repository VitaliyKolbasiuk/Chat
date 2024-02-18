#ifndef CHAT_SETTINGSDIALOG_H
#define CHAT_SETTINGSDIALOG_H

#include <QDialog>
#include <Client/ChatClient.h>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog {
Q_OBJECT

public:
    explicit SettingsDialog(ChatClient& chatClient, bool isNewUser, QWidget *parent = nullptr);

    ~SettingsDialog() override;

private slots:
    void onSaveBtnReleased();

    void onCancelBtnReleased();

private:
    Ui::SettingsDialog  *ui;
    ChatClient&         m_chatClient;
    std::optional<bool> m_isNewUser;
};

#endif //CHAT_SETTINGSDIALOG_H
