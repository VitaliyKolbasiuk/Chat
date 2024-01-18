#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Client/ClientInterfaces.h"
#include "Client/ChatClient.h"
#include "Client/TcpClient.h"
#include "Settings.h"
#include "Utils.h"
#include "CreateChatRoom.h"
#include "SettingsDialog.h"

#include <QDir>
#include <QTextBrowser>

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
    m_chatClient = std::make_shared<QChatClient>(*m_settings);

    connect(m_chatClient.get(), &QChatClient::OnMessageReceived, this, [this](ChatRoomId chatRoomId, const std::string& username, const std::string& message, uint64_t time){
        int year, month, day, hour, minute, second;
        parseUtcTime(time, year, month, day, hour, minute, second);

        for (int i = 0; i < ui->m_chatRoomList->count(); i++)
        {
            if (ui->m_chatRoomList->item(i)->data(Qt::UserRole) == chatRoomId.m_id)
            {
                ui->m_chatRoomList->setCurrentRow(i);

                QListWidgetItem *newItem = new QListWidgetItem;
                QTextBrowser *textBrowser = new QTextBrowser;
                QVariant messageId(ui->m_chatRoomArea->count() + 1);
                newItem->setData(Qt::UserRole, messageId);

                QString htmlText = QString::fromStdString(username) + "<br>" +
                                   QString::fromStdString(message) + "<br>" +
                                   QString::fromStdString(std::to_string(hour)) + ':' +
                                   QString::fromStdString(std::to_string(minute));

                textBrowser->setHtml(htmlText);
                newItem->setSizeHint(textBrowser->sizeHint());

                ui->m_chatRoomArea->addItem(newItem);
                ui->m_chatRoomArea->setItemWidget(newItem, textBrowser);


                break;
            }
        }
    });

    connect(m_chatClient.get(), &QChatClient::OnChatRoomAddedOrDeleted, this, [this](const ChatRoomId& chatRoomId, const std::string& chatRoomName, bool isAdd){
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
                if (item->data(Qt::UserRole).toUInt() == 5)
                {
                    int row = ui->m_chatRoomList->row(item);
                    ui->m_chatRoomList->takeItem(row);
                    delete item;
                }
            }
        }
    });

    connect(m_chatClient.get(), &QChatClient::OnTableChanged, this, [this](const ChatRoomMap& chatRoomInfoList){
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
    ui->TextChatRoomName->setStyleSheet(style);
    ui->TextUsername->setStyleSheet(style);
    ui->Username->setStyleSheet(style);
    ui->ChatRoomName->setStyleSheet(style);
    ui->Exit->setStyleSheet(style);
    ui->Join->setStyleSheet(style);
    ui->m_chatRoomArea->setStyleSheet(style);
    ui->SaveSettings->setStyleSheet(style);
    ui->UserMessage->setStyleSheet(style);
    ui->SendMessage->setStyleSheet(style);
    ui->m_CreateRoom->setStyleSheet(style);

    ui->m_chatRoomList->setStyleSheet(style);

}

void MainWindow::onChatRoomListReceived()
{

}

void MainWindow::on_Join_released()
{
    QString chatRoomName = ui->TextChatRoomName->text();
    QString username = ui->TextUsername->text();
    if (!chatRoomName.isEmpty() && !username.isEmpty())
    {
        //m_chatClient->sendUserMessage(JOIN_TO_CHAT_CMD ";");
        ui->TextChatRoomName->setVisible(false);
        ui->TextUsername->setVisible(false);
        ui->Join->setVisible(false);
        ui->ChatRoomName->setVisible(false);
        ui->Username->setVisible(false);
        ui->SaveSettings->setVisible(false);
        ui->SendMessage->setVisible(true);
        ui->UserMessage->setVisible(true);
    }
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

