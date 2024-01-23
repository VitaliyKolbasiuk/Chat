#include "ChatInterfaces.h"
#include "Types.h"
#include "ServerSession.h"
#include "Protocol.h"

#include <iostream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <filesystem>

struct Query{
    QSqlQuery m_query;
    Query(const QSqlDatabase& db) : m_query(db) {}

    bool prepare(const std::string& sqlQuery)
    {
        qDebug() << "SQLQuery prepare: " << sqlQuery;
        if (!m_query.prepare(QString::fromStdString(sqlQuery)))
        {
            qCritical() << "Prepare query ERROR: " << m_query.lastError().text();
            return false;
        }
        return true;
    }

    bool exec()
    {
        qDebug() << "Query: " << m_query.lastQuery();
        if (!m_query.exec())
        {
            qCritical() << "Exec query ERROR: " << m_query.lastError().text();
            return false;
        }
        return true;
    }

    int lastInsertId()
    {
        return m_query.lastInsertId().toInt();
    }

    void bindValue(const QString& placeholder, const std::string& value)
    {
        m_query.bindValue(placeholder, QString::fromStdString(value));
    }

    void bindValue(const QString& placeholder, const int& value)
    {
        m_query.bindValue(placeholder, value);
    }

    bool next()
    {
        if (!m_query.next())
        {
            if (!m_query.lastError().text().isEmpty())
            {
                qCritical() << "next query ERROR: " << m_query.lastError().text();
            }
            return false;
        }
        return true;
    }

    QVariant value(int i)
    {
        return m_query.value(i);
    }
};

class ChatDatabase : public IChatDatabase
{
    QSqlDatabase m_db;
    IChat&       m_chat;

public:
    ChatDatabase(IChat& chat) : m_chat(chat) {
        //qDebug() << QSqlDatabase::drivers();
        //std::filesystem::remove("ChatDatabase.sqlite");

        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setHostName("localhost");
        m_db.setDatabaseName("ChatDatabase.sqlite");
        m_db.setUserName("ChatServer");
        m_db.setPassword("24681234");

        if (!m_db.open())
        {
            qDebug() << m_db.lastError().text();
        }

        createChatRoomCatalogue();
        //fillTestData();
    }

    // TODO unread messages in db

    std::string chatRoomTableName(int id)
    {
        return "ChatRoomTable_" + std::to_string(id);
    }


    virtual int createChatRoomTable(const std::string& chatRoomName, bool isPrivate, Key ownerPublicKey,  std::weak_ptr<ServerSession> session) override
    {
        Query query(m_db);

        query.prepare("INSERT INTO ChatRoomCatalogue (chatRoomName, chatRoomTableName, isPrivate, ownerPublicKey) "
                      "VALUES (:chatRoomName, "
                      ":chatRoomTableName, "
                      ":isPrivate, "
                      ":ownerPublicKey)");
        query.bindValue(":chatRoomName", chatRoomName);
        query.bindValue(":isPrivate", isPrivate);
        query.bindValue(":ownerPublicKey", toString(ownerPublicKey));
        query.exec();

        int chatRoomId = query.lastInsertId();

        std::string tableName = chatRoomTableName(chatRoomId);

        query.prepare("CREATE TABLE IF NOT EXISTS " + tableName + "_members "
                                                                  "("
                                                                  "userId INT UNIQUE, "
                                                                  "FOREIGN KEY (userId) REFERENCES UserCatalogue(userId)"
                                                                  ");");
        query.exec();

        query.prepare("CREATE TABLE IF NOT EXISTS " + tableName + " "
                                                                  "("
                                                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                                                  "message TEXT, "
                                                                  "senderIdRef INT, "
                                                                  "time BIGINT, "
                                                                  "FOREIGN KEY (senderIdRef) REFERENCES UserCatalogue(userId)"
                                                                  ");");
        query.exec();

        query.prepare("INSERT INTO " + tableName + "_members (userId) VALUES ((SELECT userId FROM UserCatalogue WHERE publicKey = '" + toString(ownerPublicKey) + "'));");
        query.exec();

        m_chat.updateChatRoomList(chatRoomName, chatRoomId, true, session);

        return chatRoomId;
    }


    void createChatRoomCatalogue()
    {
        Query query(m_db);
        query.prepare("CREATE TABLE IF NOT EXISTS ChatRoomCatalogue \
                                   (\
                                        chatRoomId INTEGER PRIMARY KEY AUTOINCREMENT, \
                                        chatRoomName TEXT,\
                                        chatRoomTableName TEXT,\
                                        ownerPublicKey TEXT VARCHAR(32),\
                                        isPrivate INT\
                                   );");
        query.exec();

        query.prepare("CREATE TABLE IF NOT EXISTS UserCatalogue \
                                   (\
                                        userId INTEGER PRIMARY KEY AUTOINCREMENT, \
                                        publicKey CHARACTER(64) UNIQUE,\
                                        deviceKey CHARACTER(64), \
                                        nickname TEXT\
                                   );");
        query.exec();

        //query.exec("INSERT INTO ChatRoomCatalogue (chatRoomName, chatRoomTableName) VALUES ('Room1', 'Room1_members');");
    }

    virtual uint32_t appendMessageToChatRoom(int chatRoomId, const Key& publicKey, uint64_t dataTime, const std::string& message, int& senderId) override
    {
        if (getUserId(publicKey, senderId))
        {
            Query query(m_db);
            std::string chatRoomTableName = getChatRoomTableName(chatRoomId);
            query.prepare("INSERT INTO " + chatRoomTableName + " (message, senderIdRef, time) VALUES ( :message, :senderId, :time);");
            query.bindValue(":message", message);
            query.bindValue(":senderId", senderId);
            query.bindValue(":time", dataTime);
            query.exec();

            int messageId = query.lastInsertId();
            qDebug() << "Message id:" << messageId;
            return messageId;
        }
        else
        {
            qCritical() << "Corrupted database";
            return std::numeric_limits<uint32_t>::max();
        }

    }

    int getChatRoomId(const std::string& chatRoomName)
    {
        Query query(m_db);
        query.prepare("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomTableName = '" + chatRoomName + "' ;");
        query.exec();
        query.next();
        auto chatRoomId = query.value(0).toInt();
        qDebug() << query.value(0);
        return chatRoomId;
    }

    virtual bool getUserId(const Key& publicKey, int& userId) override
    {
        Query query(m_db);
        bool ok;
        query.prepare("SELECT userId FROM UserCatalogue WHERE publicKey = '" + toString((std::array<uint8_t, 32>&)publicKey) + "';");
        query.exec();
        query.next();
        userId = query.value(0).toInt(&ok);
        return ok;
    }

    std::string getChatRoomTableName(int chatRoomId)
    {
        Query query(m_db);
        query.prepare("SELECT chatRoomTableName FROM ChatRoomCatalogue WHERE chatRoomId = " + std::to_string(chatRoomId) + " ;");
        query.exec();
        query.next();
        return query.value(0).toString().toStdString();
    }

    void onUserConnected(const Key& publicKey, const Key& deviceKey, const std::string& nickname, std::function<void(const ChatRoomInfoList&)> func) override
    {
        Query query (m_db);
        query.prepare("INSERT INTO UserCatalogue (publicKey , deviceKey, nickname) VALUES (:publicKey, :deviceKey, :nickname);");
        query.bindValue(":publicKey", toString((std::array<uint8_t, 32>&)publicKey));
        query.bindValue(":deviceKey",  toString((std::array<uint8_t, 32>&)deviceKey));
        query.bindValue(":nickname", nickname);
        query.exec();

        int userId;
        if (getUserId(publicKey, userId))
        {
            ChatRoomInfoList chatRoomList = getChatRoomList(userId);

            func(chatRoomList);
        }
        else
        {
            qCritical() << "Corrupted database";
        }
    }

    virtual void onRequestMessages(ChatRoomId chatRoomId, int messageNumber, MessageId messageId, std::function<void(const std::vector<ChatRoomRecord>&)> func) override
    {
        Query query(m_db);
        std::string chatRoomTableName = getChatRoomTableName(chatRoomId.m_id);
        query.prepare("SELECT id, message, senderIdRef, time  FROM " + chatRoomTableName + " ORDER BY id DESC WHERE id < " + std::to_string(messageId.m_id) + ";");
        query.exec();

        std::vector<ChatRoomRecord> records;
        for (int i = 0; i < messageNumber; i++)
        {
            query.next();
            int id = query.value(0).toInt();
            std::string message = query.value(1).toString().toStdString();
            int senderId = query.value(2).toInt();
            int time = query.value(3).toInt();
            records.emplace_back(id, time, senderId, message);
        }
        func(records);
    }

    ChatRoomInfoList getChatRoomList(const int& userId) override
    {
        ChatRoomInfoList chatRooms;

        Query query(m_db);
        query.prepare("SELECT chatRoomId, chatRoomName, chatRoomTableName FROM ChatRoomCatalogue;");
        query.exec();
        while (query.next())
        {
            uint32_t chatRoomId = query.value(0).toUInt();
            QString chatRoomName = query.value(1).toString();
            QString chatRoomTableName = query.value(2).toString();

            Query query2(m_db);
            query2.prepare("SELECT userId FROM " + chatRoomTableName.toStdString() + "_members WHERE userId = :userId;");
            query2.bindValue(":userId", userId);
            query2.exec();
            while (query2.next())
            {
                chatRooms.emplace_back(chatRoomId, chatRoomName.toStdString());
            }
        }
        return chatRooms;
    }

    virtual void readChatRoomCatalogue(std::function<void(uint32_t, const std::string&, const std::string&, const Key&, bool)> func) override
    {
        Query query(m_db);
        query.prepare("SELECT chatRoomId, chatRoomName, chatRoomTableName, ownerPublicKey, isPrivate FROM ChatRoomCatalogue;");
        query.exec();
        while (query.next())
        {
            uint32_t chatRoomId = query.value(0).toUInt();
            std::string chatRoomName = query.value(1).toString().toStdString();
            std::string chatRoomTableName = query.value(2).toString().toStdString();
            Key publicKey = parseHexString<32>(query.value(3).toString().toStdString());
            bool isPrivate = query.value(4).toBool();

            func(chatRoomId, chatRoomName, chatRoomTableName, publicKey, isPrivate);
        }
    }


    void test() override
    {
//        Query query(m_db);
//        query.exec("SELECT * FROM ChatRoomCatalogue;");
//        while (query.m_query.next())
//        {
//            qDebug() << query.value(0).toString();
//            qDebug() << query.value(1).toString();
//        }

        //getUserId({});
        //createChatRoomTable("Room1");
//        createChatRoomTable("Room2");
//        query.m_query.next();
//        qDebug() << query.value(0).toString();
//        qDebug() << query.value(1).toString();
//        int chatRoomId = getChatRoomId("Room1");
//        appendMessageToChatRoom(chatRoomId, "Welcome to the server", 1, 224000 );
    }

};

IChatDatabase* createDatabase(IChat& chat)
{
    return (IChatDatabase*)new ChatDatabase(chat);
}
