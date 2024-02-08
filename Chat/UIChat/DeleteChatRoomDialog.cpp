#include "DeleteChatRoomDialog.h"
#include "ui_DeleteChatRoomDialog.h"


DeleteChatRoomDialog::DeleteChatRoomDialog(ChatClient& chatClient, ChatRoomId chatRoomId, QWidget *parent) :
        QDialog(parent), ui(new Ui::DeleteChatRoomDialog),
        m_chatClient(chatClient),
        m_chatRoomId(chatRoomId)
{
    ui->setupUi(this);

    connect(ui->m_cancel, &QPushButton::released, this, &DeleteChatRoomDialog::on_cancelBtn_released);
    connect(ui->m_delete, &QPushButton::released, this, &DeleteChatRoomDialog::on_deleteBtn_released);
}

DeleteChatRoomDialog::~DeleteChatRoomDialog()
{
    delete ui;
}

void DeleteChatRoomDialog::on_cancelBtn_released()
{
    QDialog::reject();
}

void DeleteChatRoomDialog::on_deleteBtn_released()
{
    m_chatClient.sendDeleteChatRoomRequest(m_chatRoomId, false);
    QDialog::accept();
}
