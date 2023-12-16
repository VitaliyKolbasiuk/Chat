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
    Query(QSqlDatabase db) : m_query(db) {}

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

    bool next()
    {
        return m_query.next();
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


    void createChatRoomTable(const std::string& chatRoomTableName)
    {
        Query query(m_db);
        query.prepare("CREATE TABLE IF NOT EXISTS " + chatRoomTableName + "_members "
                                                                     "("
                                                                         "senderId INTEGER PRIMARY KEY AUTOINCREMENT, "
                                                                         "userId INT, "
                                                                         "FOREIGN KEY (userId) REFERENCES UserCatalogue(userId)"
                                                                     ");");
        query.prepare("CREATE TABLE IF NOT EXISTS " + chatRoomTableName + " "
                                                                     "("
                                                                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                                                        "message TEXT, "
                                                                        "senderIdRef INT, "
                                                                        "time BIGINT, "
                                                                        "FOREIGN KEY (senderIdRef) REFERENCES UserCatalogue(userId)"
                                                                     ");");
        query.prepare("INSERT INTO ChatRoomCatalogue (chatRoomTableName) VALUES ('" + chatRoomTableName + "');");
    }

    void createChatRoomCatalogue()
    {
        Query query(m_db);
        query.prepare("CREATE TABLE IF NOT EXISTS ChatRoomCatalogue \
                                   (\
                                        chatRoomId INTEGER PRIMARY KEY AUTOINCREMENT, \
                                        chatRoomName TEXT,\
                                        chatRoomTableName TEXT, \
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

    void appendMessageToChatRoom(int chatRoomId, const std::string& message, int senderId, uint64_t dataTime)
    {
        Query query(m_db);
        query.prepare("SELECT chatRoomTableName FROM ChatRoomCatalogue WHERE chatRoomId = " + std::to_string(chatRoomId) + " ;");
        auto chatRoomTableName = query.value(0).toString().toStdString();
        query.prepare("INSERT INTO " + chatRoomTableName + " (message, senderIdRef, time) VALUES ( '" + message + "', " + std::to_string(senderId) + ", " + std::to_string(dataTime) + ");");
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
        query.prepare("SELECT userId FROM UserCatalogue WHERE publicKey = '" + toString((std::array<uint8_t, 32>&)publicKey) + "' ;");
        userId = query.value(0).toInt(&ok);
        return ok;
    }

    void onUserConnected(const Key& publicKey, const Key& deviceKey, const std::string& nickname, std::weak_ptr<ServerSession> session) override
    {
        Query query (m_db);
        query.prepare("INSERT INTO UserCatalogue (publicKey , deviceKey, nickname) VALUES (:publicKey, :deviceKey, :nickname);");
        query.bindValue(":publicKey", toString((std::array<uint8_t, 32>&)publicKey));
        query.bindValue(":deviceKey",  toString((std::array<uint8_t, 32>&)deviceKey));
        query.bindValue(":nickname", nickname);

        query.prepare("SELECT userId FROM UserCatalogue WHERE publicKey = :publickey");
        query.bindValue(":publicKey", toString((std::array<uint8_t, 32>&)publicKey));

        if (query.exec())
        {
            query.next();
            int userId = query.value(0).toInt();
            std::vector<std::string> chatRoomList = getChatRoomList(userId);

            boost::asio::post( gServerIoContext, [=, this]() mutable
            {
                if (const auto& sessionPtr = session.lock(); sessionPtr)
                {
                    ChatRoomListPacket* packet = createChatRoomList(chatRoomList);
                    sessionPtr->sendBufferedPacket<ChatRoomListPacket>(packet);
                }
            } );
        }
        else
        {
            qCritical() << "Corrupted database";
        }
    }

    std::vector<std::string> getChatRoomList(const int& userId) override
    {
        std::vector<std::string> chatRooms;

        Query query(m_db);
        query.prepare("SELECT cr.chatRoomId, cr.chatRoomName, cr.chatRoomTableName FROM ChatRoomCatalogue cr;");
        query.exec();
        while (query.m_query.next())
        {
            QString chatRoomTableName = query.value(2).toString();
            Query query2(m_db);
            query2.prepare("SELECT userId FROM " + chatRoomTableName.toStdString() + "_members WHERE userId = " + std::to_string(userId) + ";");
            query2.exec();
            while (query2.m_query.next())
            {
                chatRooms.push_back(chatRoomTableName.toStdString());
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
//        createChatRoomTable("Room1");
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
