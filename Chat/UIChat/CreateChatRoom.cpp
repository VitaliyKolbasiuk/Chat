#include "CreateChatRoom.h"
#include "ui_CreateChatRoom.h"
#include "Client/ClientInterfaces.h"
#include "Client/ChatClient.h"

#include <QMessageBox>

CreateChatRoom::CreateChatRoom(QChatClient& chatClient, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateChatRoom),
    m_chatClient(chatClient)
{
    ui->setupUi(this);
}

CreateChatRoom::~CreateChatRoom()
{
    delete ui;
}

void CreateChatRoom::on_buttonBox_accepted()
{
    if(ui->m_chatRoomName->text().trimmed().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("ChatRoomName is empty");
        msgBox.exec();
        return;
    }

    m_chatClient.createChatRoom(ui->m_chatRoomName->text().trimmed().toStdString(), ui->m_privateCBox->isChecked());
}

