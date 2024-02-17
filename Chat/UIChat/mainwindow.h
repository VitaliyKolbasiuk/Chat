#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Client/ChatClient.h"
#include "Client/TcpClient.h"

class Settings;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Ui::MainWindow *ui;
    Settings* m_settings;

    std::shared_ptr<ChatClient> m_chatClient;
    io_context                  m_ioContext1;
    std::shared_ptr<TcpClient>  m_tcpClient;
    std::optional<MessageId>    m_editedMessageId;
public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void init();
    void showMessage(MessageId messageId, const std::string& username, const std::string& message, uint64_t hour, uint64_t minute);

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void doUpdateChatRoomRecords(ChatRoomId chatRoomId);
    void onEnterKeyReleased();
    void configureUI();

private slots:

    void on_SaveSettings_released();

    void on_SendMessage_released();

    void on_Exit_released();

    void on_ConnectToChatRoomBtn_released();

    void on_m_CreateRoom_released();

    void on_m_deleteRoomBtn_released();

    void onChatRoomListReceived();

    void onMessageEdited();
};
#endif // MAINWINDOW_H
