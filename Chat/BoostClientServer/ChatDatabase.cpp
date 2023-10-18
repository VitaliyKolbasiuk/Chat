#pragma once

#include "ChatInterfaces.h"

#include <iostream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#define CHECK_DB(expr) {expr; if (!query.isValid()) qDebug() << "Error: " << query.lastError().text();}

class ChatDatabase : public IChatDatabase
{
    QSqlDatabase m_db;
public:
    ChatDatabase() {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setHostName("localhost");
        m_db.setDatabaseName("ChatDatabase");
        m_db.setUserName("ChatServer");
        m_db.setPassword("24681234");
        bool ok = m_db.open();
        if (!ok)
        {
            qDebug() << m_db.lastError().text();
        }
    }

    void createChatRoomTable(const std::string& chatRoomName)
    {
        QSqlQuery query;
        CHECK_DB(query.exec(QString::fromStdString("CREATE TABLE IF NOT EXISTS " + chatRoomName + "_members (senderId INT AUTO_INCREMENT not null PRIMARY KEY, username TEXT);")));
        CHECK_DB(query.exec(QString::fromStdString("CREATE TABLE IF NOT EXISTS " + chatRoomName + " (id INT not null AUTO_INCREMENT PRIMARY KEY, message TEXT,INDEX senderIdRef INT, time BIGINT, FOREIGN KEY (senderIdRef) REFERENCES " + chatRoomName + "_members(senderId));")));
        CHECK_DB(query.exec(QString::fromStdString("INSERT INTO ChatRoomCatalogue (chatRoomName) VALUES (\'" + chatRoomName + "\');")));
    }

    void createChatRoomCatalogue()
    {
        QSqlQuery query;
        CHECK_DB(query.exec(QString::fromStdString("CREATE TABLE IF NOT EXISTS ChatRoomCatalogue (chatRoomId INT AUTO_INCREMENT not null PRIMARY KEY, chatRoomName TEXT, chatRoomTableName TEXT);")));
        CHECK_DB(query.exec(QString::fromStdString("CREATE TABLE IF NOT EXISTS UserCatalogue (userId INT AUTO_INCREMENT not null PRIMARY KEY, username TEXT);")));
    }

    void appendMessageToChatRoom(int chatRoomId, const std::string& message, int senderId, uint64_t dataTime)
    {
        QSqlQuery query;
        CHECK_DB(query.exec(QString::fromStdString("SELECT chatRoomTableName FROM ChatRoomCatalogue WHERE chatRoomId = " + std::to_string(chatRoomId) + " ;")));
        auto chatRoomTableName = query.value(0).toString().toStdString();
        CHECK_DB(query.exec(QString::fromStdString("INSERT INTO " + chatRoomTableName + " (message, senderIdRef, time) VALUES ( " + message + ", " + std::to_string(senderId) + ", " + std::to_string(dataTime) + ");")));
    }

    int getChatRoomId(const std::string& chatRoomName)
    {
        QSqlQuery query;
        CHECK_DB(query.exec(QString::fromStdString("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomName = " + chatRoomName + " ;")));
        auto chatRoomId = query.value(0).toInt();
        return chatRoomId;
    }

    int getUserId(const std::string& username)
    {
        QSqlQuery query;
        CHECK_DB(query.exec(QString::fromStdString("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomName = " + username + " ;")));
        auto userId = query.value(0).toInt();
        return userId;
    }

    void test() override
    {
        createChatRoomCatalogue();
        createChatRoomTable("Room1");
        createChatRoomTable("Room2");
        int chatRoomId = getChatRoomId("Room1");
        //appendMessageToChatRoom(chatRoomId, "Welcome to the server", )
    }
};

IChatDatabase* createDatabase()
{
    return (IChatDatabase*)new ChatDatabase;
}
