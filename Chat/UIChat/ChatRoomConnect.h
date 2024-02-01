//
// Created by vitaliykolbasiuk on 27.01.24.
//

#ifndef CHAT_CHATROOMCONNECT_H
#define CHAT_CHATROOMCONNECT_H

#include <QDialog>

#include "Client/ChatClient.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class ChatRoomConnect;
}
QT_END_NAMESPACE

class ChatRoomConnect : public QDialog
{
    Q_OBJECT

public:
    explicit ChatRoomConnect(ChatClient& chatClient, QWidget *parent = nullptr);

    ~ChatRoomConnect() override;

//private slots:
    void on_connectBtn_released();

    void on_cancelBtn_released();

private:
    Ui::ChatRoomConnect *ui;

    ChatClient& m_chatClient;
};


#endif //CHAT_CHATROOMCONNECT_H
