#include "ChatRoomConnect.h"
#include "ui_ChatRoomConnect.h"

ChatRoomConnect::ChatRoomConnect(ChatClient& chatClient, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::ChatRoomConnect),
        m_chatClient(chatClient)
{
    ui->setupUi(this);

    connect(ui->m_connect, &QPushButton::released, this, [this]{
        on_connectBtn_released();
    });
    connect(ui->m_cancel, &QPushButton::released, this, [this]{
        on_cancelBtn_released();
    });
}

ChatRoomConnect::~ChatRoomConnect()
{
    delete ui;
}

void ChatRoomConnect::on_connectBtn_released()
{
    if (!ui->m_chatRoomName->text().trimmed().isEmpty())
    {
        std::string chatRoomName = ui->m_chatRoomName->text().toStdString();

        if (m_chatClient.connectToChatRoom(chatRoomName))
        {
            QDialog::accept();
        }
        else
        {
            qDebug() << "Incorrect chat room name";
            QDialog::reject();
        }
    }
}

void ChatRoomConnect::on_cancelBtn_released()
{
    QDialog::reject();
}
