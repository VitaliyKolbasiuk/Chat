#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Client/ClientInterfaces.h"
#include "Client/ChatClient.h"
#include "Client/TcpClient.h"
#include "Settings.h"
#include "Utils.h"
#include "CreateChatRoom.h"
#include "SettingsDialog.h"
#include "ChatRoomConnect.h"
#include "DeleteChatRoomDialog.h"
#include "LeaveChatRoomDialog.h"

#include <QTextBrowser>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    configureUI();
    init();
}

void MainWindow::init()
{
    m_settings = new Settings();
    if (!m_settings->loadSettings())
    {
        SettingsDialog settingsDialog;
        settingsDialog.setModal(true);
        if (settingsDialog.exec() == QDialog::Accepted)
        {
            if (!m_settings->loadSettings())
            {
                qDebug() << "Load Settings failed";
            }
        }
        else
        {
            exit(0);
        }
    }
    m_chatClient = std::make_shared<ChatClient>(*m_settings);

    connect(ui->m_chatRoomList, &QListWidget::currentRowChanged, this, [this](int currentRow){
        if (auto item = ui->m_chatRoomList->currentItem(); item != nullptr)
        {
            doUpdateChatRoomRecords((ChatRoomId)item->data(Qt::UserRole).toUInt());
        }
    });

    connect(m_chatClient.get(), &ChatClient::OnMessageReceived, this, [this](ChatRoomId chatRoomId, MessageId messageId, const std::string& username, const std::string& message, uint64_t time){
        if (ui->m_chatRoomList->currentItem()->data(Qt::UserRole).toUInt() == chatRoomId.m_id)
        {
            int year, month, day, hour, minute, second;
            parseUtcTime(time, year, month, day, hour, minute, second);

            for (int i = 0; i < ui->m_chatRoomList->count(); i++)
            {
                if (ui->m_chatRoomList->item(i)->data(Qt::UserRole) == chatRoomId.m_id)
                {
                    showMessage(messageId, username,message, hour, minute);
                    break;
                }
            }
        }
    });

    connect(m_chatClient.get(), &ChatClient::OnChatRoomAddedOrDeleted, this, [this](const ChatRoomId chatRoomId, const std::string& chatRoomName, bool isAdd){
        QListWidgetItem *newItem = new QListWidgetItem;
        QVariant id(chatRoomId.m_id);
        newItem->setData(Qt::UserRole, id);
        newItem->setText(QString::fromStdString(chatRoomName));

        if (isAdd)
        {
            ui->m_chatRoomList->insertItem(0, newItem);
            ui->m_chatRoomList->setCurrentRow(0);
        }
        else
        {
            QList<QListWidgetItem *> itemsToRemove = ui->m_chatRoomList->findItems(QString::fromStdString(chatRoomName), Qt::MatchExactly);

            for (auto item : itemsToRemove)
            {
                if (item->data(Qt::UserRole).toUInt() == chatRoomId.m_id)
                {
                    int row = ui->m_chatRoomList->row(item);
                    ui->m_chatRoomList->takeItem(row);
                    delete item;
                }
            }
        }
    });

    connect(m_chatClient.get(), &ChatClient::OnTableChanged, this, [this](const ChatRoomMap& chatRoomInfoList){
        ui->m_chatRoomList->clear();
        for (const auto& [key, chatRoomInfo] : chatRoomInfoList)
        {
            QListWidgetItem *newItem = new QListWidgetItem;
            QVariant chatRoomId(chatRoomInfo.m_id.m_id);
            newItem->setData(Qt::UserRole, chatRoomId);
            newItem->setText(QString::fromStdString(chatRoomInfo.m_name));
            ui->m_chatRoomList->addItem(newItem);

        }
    });

    connect(ui->m_chatRoomList, &QListWidget::currentItemChanged, this, [this](const QListWidgetItem* item){
        if (item != 0) {
            QVariant data = item->data(Qt::UserRole);
            int chatRoomId = data.toInt();
        }
    });

    connect(m_chatClient.get(), &ChatClient::updateChatRoomRecords, this, [this](ChatRoomId chatRoomId){
        doUpdateChatRoomRecords(chatRoomId);
    });
    
    connect(ui->m_connectToChatRoomBtn, &QPushButton::released, this, &MainWindow::on_ConnectToChatRoomBtn_released);

    connect(ui->m_deleteRoom, &QPushButton::released, this, &MainWindow::on_m_deleteRoomBtn_released);





    //qDebug() << QDir::homePath();
    //system("dir");
    ui->TextUsername->setText(QString::fromStdString(m_settings->m_username));

    m_tcpClient = std::make_shared<TcpClient>(m_ioContext1, m_chatClient);
    m_chatClient->setTcpClient(m_tcpClient);
    m_tcpClient->connect("127.0.0.1", 1234);

    //m_chatClient->sendUserMessage(CONNECT_CMD ";" + arrayToHexString(m_settings->m_userKey) + ";" + arrayToHexString(m_settings->m_deviceKey) + ";");

    std::thread ([this]{
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(m_ioContext1.get_executor());
        m_ioContext1.run();
        qDebug() << "Context has stopped";
    }).detach();

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_settings;
}

void MainWindow::configureUI()
{
    QString style = "background-color: #282e33; color : #e9f2f4; font : bold; QLabel { text-align: center; }";

    ui->centralwidget->setStyleSheet("background-color: #18191d");
    ui->TextUsername->setStyleSheet(style);
    ui->Username->setStyleSheet(style);
    ui->Exit->setStyleSheet(style);
    ui->m_chatRoomArea->setStyleSheet(style);
    ui->SaveSettings->setStyleSheet(style);
    ui->UserMessage->setStyleSheet(style);
    ui->SendMessage->setStyleSheet(style);
    ui->m_CreateRoom->setStyleSheet("background-color: #282e33; color : lightgreen; font : bold; QLabel { text-align: center; }");
    ui->m_deleteRoom->setStyleSheet("background-color: #282e33; color : red; font : bold; QLabel { text-align: center; }");

    ui->m_chatRoomList->setStyleSheet(style);

}

void MainWindow::onChatRoomListReceived()
{

}

void MainWindow::on_SaveSettings_released()
{
    m_settings->m_username = ui->TextUsername->text().toStdString();
    m_settings->saveSettings();
}


void MainWindow::on_SendMessage_released()
{
    std::string userMessage = ui->UserMessage->text().toStdString();
    if (!userMessage.empty())
    {
        ChatRoomId chatRoomId((uint64_t)ui->m_chatRoomList->currentItem()->data(Qt::UserRole).toULongLong());
        auto* packet = createSendTextMessagePacket(userMessage, chatRoomId, m_settings->m_keyPair.m_publicKey);
        m_tcpClient->sendBufferedPacket<SendTextMessagePacket>(packet);

        ui->UserMessage->clear();
    }
}

void MainWindow::on_m_deleteRoomBtn_released()
{
    if (ui->m_chatRoomList->currentItem() != nullptr)
    {
        auto chatRoomInfo = m_chatClient->getChatRoomMap().find(static_cast<ChatRoomId>(ui->m_chatRoomList->currentItem()->data(Qt::UserRole).toUInt()));
        ChatRoomId chatRoomId = static_cast<ChatRoomId>(ui->m_chatRoomList->currentItem()->data(Qt::UserRole).toUInt());
        if (chatRoomInfo->second.m_isOwner)
        {
            DeleteChatRoomDialog deleteChatRoomDialog(*m_chatClient, chatRoomId);
            deleteChatRoomDialog.setModal(true);
            deleteChatRoomDialog.exec();
        }
        else
        {
            LeaveChatRoomDialog leaveChatRoomDialog(*m_chatClient, chatRoomId);
            leaveChatRoomDialog.setModal(true);
            leaveChatRoomDialog.exec();
        }


    }
}


void MainWindow::on_Exit_released()
{
    m_chatClient->closeConnection();
}



void MainWindow::on_m_CreateRoom_released()
{
    CreateChatRoom createChatRoom(*m_chatClient, nullptr);
    createChatRoom.setModal(true);
    createChatRoom.exec();
}

void MainWindow::on_ConnectToChatRoomBtn_released()
{
    ChatRoomConnect chatRoomConnect(*m_chatClient, nullptr);
    chatRoomConnect.setModal(true);
    chatRoomConnect.exec();
}

void MainWindow::doUpdateChatRoomRecords(ChatRoomId chatRoomId)
{
    auto* currentItem =  ui->m_chatRoomList->currentItem();
    if (currentItem == nullptr)
    {
        return;
    }

    int currentChatRoomId = currentItem->data(Qt::UserRole).toInt();
    if (currentChatRoomId != chatRoomId.m_id)
    {
        return;
    }

    auto& chatRoomData = m_chatClient->getChatRoomMap()[chatRoomId];

    ui->m_chatRoomArea->clear();
    for (const auto& [key, record] : chatRoomData.m_records)
    {
        int year, month, day, hour, minute, second;
        parseUtcTime(record.m_time, year, month, day, hour, minute, second);
        showMessage(record.m_messageId, record.m_username, record.m_text, hour, minute);
    }
}

void MainWindow::showMessage(MessageId messageId, const std::string& username, const std::string& message, uint64_t hour, uint64_t minute)
{
    QListWidgetItem *newItem = new QListWidgetItem;
    QTextBrowser *textBrowser = new QTextBrowser;
    newItem->setData(Qt::UserRole, messageId.m_id);

    std::string hourStr = hour > 9 ? std::to_string(hour) : '0' + std::to_string(hour);
    std::string minuteStr = minute > 9 ? std::to_string(minute) : '0' + std::to_string(minute);

    QString htmlText = QString::fromStdString(username) + "<br>" +
                       QString::fromStdString(message) + "<br>" +
                       QString::fromStdString(hourStr) + ':' +
                       QString::fromStdString(minuteStr);

    textBrowser->setHtml(htmlText);
    newItem->setSizeHint(textBrowser->sizeHint());

    QSize size(200, 70);
    newItem->setSizeHint(size);
    textBrowser->setFixedSize(size);

    ui->m_chatRoomArea->addItem(newItem);
    ui->m_chatRoomArea->setItemWidget(newItem, textBrowser);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Return)
    {
        onEnterKeyReleased();
    }

    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::onEnterKeyReleased()
{
    if (ui->UserMessage->hasFocus())
    {
        on_SendMessage_released();
    }
}




