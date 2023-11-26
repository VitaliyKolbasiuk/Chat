#include "ChatInterfaces.h"

#include <iostream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <filesystem>

struct Query{
    QSqlQuery m_query;
    Query(QSqlDatabase db) : m_query(db) {}

    void exec(const std::string& sqlQuery)
    {
        qDebug() << "sqlQuery: " << sqlQuery;
        m_query.exec(QString::fromStdString(sqlQuery));
        if (!m_query.isValid())
        {
            qDebug() << "Error: " << m_query.lastError().text();
        }
    }
    void execq(const QString& sqlQuery)
    {
        qDebug() << "sqlQuery: " << sqlQuery;
        m_query.exec(sqlQuery);
        if (!m_query.isValid())
        {
            qDebug() << "Error: " << m_query.lastError().text();
        }
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
        std::filesystem::remove("ChatDatabase.sqlite");

        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setHostName("localhost");
        m_db.setDatabaseName("ChatDatabase.sqlite");
        m_db.setUserName("ChatServer");
        m_db.setPassword("24681234");
        bool ok = m_db.open();
        if (!ok)
        {
            qDebug() << m_db.lastError().text();
        }

        //fillTestData();
    }

    void createChatRoomTable(const std::string& chatRoomName)
    {
        Query query(m_db);
        query.exec("CREATE TABLE IF NOT EXISTS " + chatRoomName + "_members (senderId INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT, userUniqueKey TEXT);");
        query.exec("CREATE TABLE IF NOT EXISTS " + chatRoomName + " (id INTEGER PRIMARY KEY AUTOINCREMENT, message TEXT, senderIdRef INT, time BIGINT, FOREIGN KEY (senderIdRef) REFERENCES " + chatRoomName + "_members(senderId));");
        query.exec("INSERT INTO ChatRoomCatalogue (chatRoomName) VALUES ('" + chatRoomName + "');");
    }

    void createChatRoomCatalogue()
    {
        Query query(m_db);
        query.exec("CREATE TABLE IF NOT EXISTS ChatRoomCatalogue \
                   (\
                   chatRoomId INTEGER PRIMARY KEY AUTOINCREMENT, \
                   chatRoomName TEXT,\
                   chatRoomTableName TEXT, \
                   ownerPublicKey TEXT VARCHAR(32),\
                   isPrivate INT\
                   );");
        query.exec("CREATE TABLE IF NOT EXISTS UserCatalogue (userId INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT);");

        //query.exec("INSERT INTO ChatRoomCatalogue (chatRoomName, chatRoomTableName) VALUES ('Room1', 'Room1_members');");
    }

    void appendMessageToChatRoom(int chatRoomId, const std::string& message, int senderId, uint64_t dataTime)
    {
        Query query(m_db);
        query.exec("SELECT chatRoomTableName FROM ChatRoomCatalogue WHERE chatRoomId = " + std::to_string(chatRoomId) + " ;");
        auto chatRoomTableName = query.value(0).toString().toStdString();
        query.exec("INSERT INTO " + chatRoomTableName + " (message, senderIdRef, time) VALUES ( '" + message + "', " + std::to_string(senderId) + ", " + std::to_string(dataTime) + ");");
    }

    int getChatRoomId(const std::string& chatRoomName)
    {
        Query query(m_db);
        query.exec("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomName = '" + chatRoomName + "' ;");
        auto chatRoomId = query.value(0).toInt();
        qDebug() << query.value(0);
        return chatRoomId;
    }

    int getUserId(const std::string& username)
    {
        Query query(m_db);
        query.exec("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomName = '" + username + "' ;");
        auto userId = query.value(0).toInt();
        return userId;
    }

    std::vector<std::string> getChatRoomList(std::string userUniqueKey) override
    {
        std::vector<std::string> chatRooms;

        Query query(m_db);
        query.exec("SELECT cr.chatRoomId, cr.chatRoomName, cr.chatRoomTableName FROM ChatRoomCatalogue cr;");
        while (query.m_query.next())
        {
            QString chatRoomTableName = query.value(2).toString();
            Query query2(m_db);
            query2.exec("SELECT userUniqueKey FROM " + chatRoomTableName.toStdString() + "_members WHERE userUniqueKey = " + userUniqueKey + ";");
            while (query2.m_query.next())
            {
                chatRooms.push_back(chatRoomTableName.toStdString() + ";" + query.value(0).toString().toStdString());
            }
        }
        return chatRooms;
    }

    void test() override
    {
        Query query(m_db);
        query.exec("SELECT * FROM ChatRoomCatalogue;");
        while (query.m_query.next()){
        qDebug() << query.value(0).toString();
        qDebug() << query.value(1).toString();
        }
//        createChatRoomCatalogue();
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