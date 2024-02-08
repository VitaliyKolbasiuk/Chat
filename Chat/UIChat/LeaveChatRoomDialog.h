#ifndef CHAT_LEAVECHATROOMDIALOG_H
#define CHAT_LEAVECHATROOMDIALOG_H

#include <QDialog>
#include "Client/ChatClient.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class LeaveChatRoomDialog;
}
QT_END_NAMESPACE

class LeaveChatRoomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LeaveChatRoomDialog(ChatClient& chatClient, ChatRoomId chatRoomId, QWidget *parent = nullptr);

    ~LeaveChatRoomDialog() override;

    void on_cancelBtn_released();

    void on_leaveBtn_released();
private:
    Ui::LeaveChatRoomDialog *ui;
    ChatClient& m_chatClient;
    ChatRoomId m_chatRoomId;
};


#endif //CHAT_LEAVECHATROOMDIALOG_H
