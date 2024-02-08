#include "LeaveChatRoomDialog.h"
#include "ui_LeaveChatRoomDialog.h"


LeaveChatRoomDialog::LeaveChatRoomDialog(ChatClient& chatClient, ChatRoomId chatRoomId,QWidget *parent) :
        QDialog(parent), ui(new Ui::LeaveChatRoomDialog),
        m_chatClient(chatClient),
        m_chatRoomId(chatRoomId)
{
    ui->setupUi(this);

    connect(ui->m_cancel, &QPushButton::released, this, &LeaveChatRoomDialog::on_cancelBtn_released);
    connect(ui->m_leave, &QPushButton::released, this, &LeaveChatRoomDialog::on_leaveBtn_released);
}

LeaveChatRoomDialog::~LeaveChatRoomDialog()
{
    delete ui;
}

void LeaveChatRoomDialog::on_cancelBtn_released()
{
    QDialog::reject();
}

void LeaveChatRoomDialog::on_leaveBtn_released()
{
    m_chatClient.sendDeleteChatRoomRequest(m_chatRoomId, true);
    QDialog::accept();
}
