
#include "ChatRoomConnect.h"
#include "ui_ChatRoomConnect.h"


ChatRoomConnect::ChatRoomConnect(QWidget *parent) :
        QDialog(parent), ui(new Ui::ChatRoomConnect)
{
    ui->setupUi(this);
}

ChatRoomConnect::~ChatRoomConnect()
{
    delete ui;
}
