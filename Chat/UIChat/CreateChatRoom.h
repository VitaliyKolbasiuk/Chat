#ifndef CREATECHATROOM_H
#define CREATECHATROOM_H

#include <QDialog>

class ChatClient;

namespace Ui {
class CreateChatRoom;
}

class CreateChatRoom : public QDialog
{
    Q_OBJECT

public:
    explicit CreateChatRoom(ChatClient& chatClient, QWidget *parent = nullptr);
    ~CreateChatRoom();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::CreateChatRoom *ui;
    ChatClient& m_chatClient;
};

#endif // CREATECHATROOM_H
