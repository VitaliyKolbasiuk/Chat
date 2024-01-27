//
// Created by vitaliykolbasiuk on 27.01.24.
//

#ifndef CHAT_CHATROOMCONNECT_H
#define CHAT_CHATROOMCONNECT_H

#include <QDialog>


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
    explicit ChatRoomConnect(QWidget *parent = nullptr);

    ~ChatRoomConnect() override;

private:
    Ui::ChatRoomConnect *ui;
};


#endif //CHAT_CHATROOMCONNECT_H
