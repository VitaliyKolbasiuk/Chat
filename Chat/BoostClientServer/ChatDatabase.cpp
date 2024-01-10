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

public:
    ChatDatabase() {
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

        //fillTestData();
    }

    // TODO unread messages in db


    virtual void createChatRoomTable(const std::string& chatRoomName, bool isPrivate, Key ownerPublicKey, std::weak_ptr<ServerSession> session) override
    {
        std::array<uint8_t, 20> randomTableName;
        generateRandomKey(randomTableName);

        std::string tableName = "t_" + toString(randomTableName);


        Query query(m_db);
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

        query.prepare("INSERT INTO ChatRoomCatalogue (chatRoomName, chatRoomTableName, isPrivate, ownerPublicKey) "
                      "VALUES (:chatRoomName, "
                      ":chatRoomTableName, "
                      ":isPrivate, "
                      ":ownerPublicKey)");
        query.bindValue(":chatRoomName", chatRoomName);
        query.bindValue(":chatRoomTableName", tableName);
        query.bindValue(":isPrivate", isPrivate);
        query.bindValue(":ownerPublicKey", toString(ownerPublicKey));
        query.exec();

        query.prepare("INSERT INTO " + tableName + "_members (userId) VALUES ((SELECT userId FROM UserCatalogue WHERE publicKey = '" + toString(ownerPublicKey) + "'));");
        query.exec();

        query.prepare("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomTableName = '" + tableName + "';");
        query.exec();
        query.next();
        uint32_t id = (uint32_t)query.value(0).toUInt();

        updateChatRoomList(chatRoomName, id, true, session);
    }

    void updateChatRoomList(const std::string& chatRoomName, uint32_t id, bool isAdd, std::weak_ptr<ServerSession> session)
    {
        PacketHeader<ChatRoomUpdatePacket> packet;
        std::memcpy(&packet.m_packet.m_chatRoomName, chatRoomName.c_str(), chatRoomName.size() + 1);
        packet.m_packet.m_chatRoomId = id;
        packet.m_packet.m_addOrDelete = isAdd;

        boost::asio::post(gServerIoContext, [=, this]() mutable
        {
            if (const auto &sessionPtr = session.lock(); sessionPtr)
            {
                sessionPtr->sendPacket(packet);
            }
        });

    }

    void createChatRoomCatalogue()
    {
        Query query(m_db);
        query.prepare("CREATE TABLE IF NOT EXISTS ChatRoomCatalogue \
                                   (\
                                        chatRoomId INTEGER PRIMARY KEY AUTOINCREMENT, \
                                        chatRoomName TEXT,\
                                        chatRoomTableName TEXT UNIQUE, \
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

    virtual void appendMessageToChatRoom(int chatRoomId, const Key& publicKey, uint64_t dataTime, const std::string& message) override
    {
        int senderId;
        if (getUserId(publicKey, senderId))
        {
            Query query(m_db);
            std::string chatRoomTableName = getChatRoomTableName(chatRoomId);
            query.prepare("INSERT INTO " + chatRoomTableName + " (message, senderIdRef, time) VALUES ( :message, :senderId, :time);");
            query.bindValue(":message", message);
            query.bindValue(":senderId", senderId);
            query.bindValue(":time", dataTime);
            query.exec();
        }
        else
        {
            qCritical() << "Corrupted database";
        }
    }

    int getChatRoomId(const std::string& chatRoomName)
    {
        Query query(m_db);
        query.prepare("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomName = '" + chatRoomName + "' ;");
        auto chatRoomId = query.value(0).toInt();
        qDebug() << query.value(0);
        return chatRoomId;
    }

    bool getUserId(const Key& publicKey, int& userId)
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

    void onUserConnected(const Key& publicKey, const Key& deviceKey, const std::string& nickname, std::weak_ptr<ServerSession> session) override
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

            boost::asio::post(gServerIoContext, [=, this]() mutable
            {
                if (const auto &sessionPtr = session.lock(); sessionPtr)
                {
                    qDebug() << "ChatRoomListPacket has been sent";
                    ChatRoomListPacket *packet = createChatRoomList(chatRoomList);
                    sessionPtr->sendBufferedPacket<ChatRoomListPacket>(packet);
                }
            });
        }
        else
        {
            qCritical() << "Corrupted database";
        }
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

    void test() override
    {
//        Query query(m_db);
//        query.exec("SELECT * FROM ChatRoomCatalogue;");
//        while (query.m_query.next())
//        {
//            qDebug() << query.value(0).toString();
//            qDebug() << query.value(1).toString();
//        }
        createChatRoomCatalogue();
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

IChatDatabase* createDatabase()
{
    return (IChatDatabase*)new ChatDatabase;
}
