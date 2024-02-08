#ifndef CHAT_DELETECHATROOMDIALOG_H
#define CHAT_DELETECHATROOMDIALOG_H

#include <QDialog>
#include "Client/ChatClient.h"


QT_BEGIN_NAMESPACE
namespace Ui
{
    class DeleteChatRoomDialog;
}
QT_END_NAMESPACE

class DeleteChatRoomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteChatRoomDialog(ChatClient& chatClient, ChatRoomId chatRoomId, QWidget *parent = nullptr);

    ~DeleteChatRoomDialog() override;

    void on_cancelBtn_released();

    void on_deleteBtn_released();

private:
    Ui::DeleteChatRoomDialog *ui;
    ChatClient& m_chatClient;
    ChatRoomId m_chatRoomId;
};


#endif //CHAT_DELETECHATROOMDIALOG_H
