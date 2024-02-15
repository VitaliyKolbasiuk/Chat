#include "ServerSession.h"

#include <QSqlQuery>
#include <QSqlError>

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
        qDebug() << "SQLQuery exec: " << m_query.lastQuery();
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
    IChatModel&       m_chat;

public:
    ChatDatabase(IChatModel& chat) : m_chat(chat) {
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

    std::string getChatRoomTableName(int id)
    {
        return "ChatRoomTable_" + std::to_string(id);
    }


    virtual int createChatRoomTable(const std::string& chatRoomName, bool isPrivate, Key ownerPublicKey,  std::weak_ptr<ServerSession> session) override
    {
        Query query(m_db);

        query.prepare("INSERT INTO ChatRoomCatalogue (chatRoomName, isPrivate, ownerPublicKey) "
                      "VALUES (:chatRoomName, "
                      ":isPrivate, "
                      ":ownerPublicKey)");
        query.bindValue(":chatRoomName", chatRoomName);
        query.bindValue(":isPrivate", isPrivate);
        query.bindValue(":ownerPublicKey", toString(ownerPublicKey));
        bool rc = query.exec();
        if (!rc)
        {
            qWarning() << "createChatRoomTable insert into ChatRoomCatalogue failed";
        }

        int chatRoomId = query.lastInsertId();
        std::string tableName = getChatRoomTableName(chatRoomId);
        qDebug() << "Create chat room table, chat room table name = " << tableName;
        qDebug() << "Create chat room table, chat room table id = " << chatRoomId;

        query.prepare("CREATE TABLE IF NOT EXISTS " + tableName + "_members "
                                                                  "("
                                                                  "userId INT UNIQUE, "
                                                                  "FOREIGN KEY (userId) REFERENCES UserCatalogue(userId)"
                                                                  ");");
        rc = query.exec();
        if (!rc)
        {
            qWarning() << "createChatRoomTable create " + tableName + "_members failed";
        }

        query.prepare("CREATE TABLE IF NOT EXISTS " + tableName + " "
                                                                  "("
                                                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                                                  "message TEXT, "
                                                                  "senderIdRef INT, "
                                                                  "time BIGINT, "
                                                                  "FOREIGN KEY (senderIdRef) REFERENCES UserCatalogue(userId)"
                                                                  ");");
        rc = query.exec();
        if (!rc)
        {
            qWarning() << "createChatRoomTable create " + tableName + " failed";
        }

        query.prepare("INSERT INTO " + tableName + "_members (userId) VALUES ((SELECT userId FROM UserCatalogue WHERE publicKey = '" + toString(ownerPublicKey) + "'));");
        rc = query.exec();
        if (!rc)
        {
            qWarning() << "createChatRoomTable insert into  " + tableName + "_members failed";
        }

        m_chat.updateChatRoomList(chatRoomName, chatRoomId, true, true, session);

        return chatRoomId;
    }


    void createChatRoomCatalogue()
    {
        Query query(m_db);
        query.prepare("CREATE TABLE IF NOT EXISTS ChatRoomCatalogue "
                      "("
                      "chatRoomId INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "chatRoomName TEXT, "
                      "ownerPublicKey TEXT VARCHAR(32), "
                      "isPrivate INT"
                      ");");
        query.exec();

        query.prepare("CREATE TABLE IF NOT EXISTS UserCatalogue "
                      "("
                      "userId INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "publicKey CHARACTER(64) UNIQUE, "
                      "deviceKey CHARACTER(64), "
                      "nickname TEXT"
                      ");");
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
            return messageId;
        }
        else
        {
            qCritical() << "Corrupted database";
            return std::numeric_limits<uint32_t>::max();
        }

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

    void onUserConnected(const Key& publicKey, const Key& deviceKey, const std::string& nickname, std::function<void(const ChatRoomInfoList&)> func) override
    {
        Query query (m_db);
        query.prepare("INSERT INTO UserCatalogue (publicKey, deviceKey, nickname) VALUES (:publicKey, :deviceKey, :nickname);");
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
        //std::string nestedSelect = "(SELECT username, userId FROM UserCatalogue WHERE userId = senderIdRef)";
        //query.prepare("SELECT id, message, senderIdRef, time, " + nestedSelect + " FROM " + chatRoomTableName + " WHERE id < " + std::to_string(messageId.m_id) + " ORDER BY id DESC;");

        //query.prepare("SELECT id, message, senderIdRef, time, UserCatalogue.nickname FROM " + chatRoomTableName + " INNER JOIN UserCatalogue ON userId = senderIdRef");

        query.prepare("SELECT id, message, senderIdRef, time, UserCatalogue.nickname FROM " + chatRoomTableName + " INNER JOIN UserCatalogue ON userId = senderIdRef WHERE id < " + std::to_string(messageId.m_id) + " ORDER BY id DESC;");


        query.exec();

        std::vector<ChatRoomRecord> records;
        for (int i = 0; query.next() && i < messageNumber; i++)
        {
            int id = query.value(0).toInt();
            std::string message = query.value(1).toString().toStdString();
            int senderId = query.value(2).toInt();
            int time = query.value(3).toInt();
            std::string username = query.value(4).toString().toStdString();
            records.emplace_back(id, time, senderId, message, username);
        }
        func(records);
    }

    ChatRoomInfoList getChatRoomList(const int& userId) override
    {
        ChatRoomInfoList chatRooms;

        Query query(m_db);
        query.prepare("SELECT chatRoomName, chatRoomId, ownerPublicKey, isPrivate FROM ChatRoomCatalogue;");
        query.exec();
        while (query.next())
        {
            std::string chatRoomName = query.value(0).toString().toStdString();
            int chatRoomId = query.value(1).toInt();
            std::string chatRoomTableName = getChatRoomTableName(chatRoomId);

            Key ownerKey;
            fromString(query.value(2).toString().toStdString(), ownerKey);

            bool isPrivate = query.value(3).toBool();

            Query query2(m_db);
            query2.prepare("SELECT userId FROM " + chatRoomTableName + "_members WHERE userId = :userId;");
            query2.bindValue(":userId", userId);
            query2.exec();
            while (query2.next())
            {
                chatRooms.emplace_back(chatRoomId, chatRoomName, ownerKey, isPrivate);
            }
        }
        return chatRooms;
    }

    int getChatRoomId(const std::string& chatRoomTableName)
    {
        size_t underscorePos = chatRoomTableName.find('_');

        if (underscorePos != std::string::npos) {
            std::string idSubstring = chatRoomTableName.substr(underscorePos + 1);

            try {
                int roomId = std::stoi(idSubstring);
                return roomId;
            } catch (const std::invalid_argument& e) {
                qCritical() << "Invalid argument: " << e.what();
            } catch (const std::out_of_range& e) {
                qCritical() << "Out of range:  " << e.what();
            }
        } else {
            qCritical() << "Underscore not found in the string.";
        }
        return -1;
    }

    virtual void readChatRoomCatalogue(std::function<void(uint32_t, const std::string&, const std::string&, const Key&, bool)> func) override
    {
        Query query(m_db);
        query.prepare("SELECT chatRoomId, chatRoomName, ownerPublicKey, isPrivate FROM ChatRoomCatalogue;");
        query.exec();
        while (query.next())
        {
            uint32_t chatRoomId = query.value(0).toUInt();
            std::string chatRoomName = query.value(1).toString().toStdString();
            std::string chatRoomTableName = getChatRoomTableName(chatRoomId);
            Key publicKey = parseHexString<32>(query.value(2).toString().toStdString());
            bool isPrivate = query.value(3).toBool();

            func(chatRoomId, chatRoomName, chatRoomTableName, publicKey, isPrivate);
        }
    }

    virtual void onConnectToChatRoomMessage(const std::string& chatRoomName, Key userKey, std::weak_ptr<ServerSession> session)
    {
        Query query(m_db);

        int userId;
        if (!getUserId(userKey, userId))
        {
            qDebug() << "Non existing user";
            return;
        }

        query.prepare("SELECT chatRoomId FROM ChatRoomCatalogue WHERE chatRoomName = '" + chatRoomName + "' "
                                                                                                         "AND isPrivate = 0 "
                                                                                                         "AND ownerPublicKey != '" + toString(userKey) + "';");
        query.exec();

        std::vector<int> chatRoomIdList;
        while (query.next())
        {
            chatRoomIdList.push_back(query.value(0).toInt());
        }

        for (const auto& chatRoomId : chatRoomIdList)
        {
            std::string chatRoomTableName = getChatRoomTableName(chatRoomId);
            query.prepare("INSERT INTO " + chatRoomTableName + "_members (userId) VALUES (" + std::to_string(userId) + ");");
            query.exec();

            m_chat.updateChatRoomList(chatRoomName, chatRoomId, true, false, session);
        }

        if (chatRoomIdList.empty())
        {
            qDebug() << "No chat room found with given name";
            m_chat.connectToChatRoomFailed(chatRoomName, session);
        }
    }

    void leaveChatRoom(ChatRoomId chatRoomId, Key userKey, bool onlyLeave) override
    {
        Query query(m_db);

        bool isOwner = checkChatRoomOwner(chatRoomId, userKey);

        if (!onlyLeave && !isOwner)
        {
            qCritical() << "leaveChatRoom: !onlyLeave && !isOwner ";
            return;
        }
        else if (onlyLeave && isOwner)
        {
            qCritical() << "leaveChatRoom: onlyLeave && isOwner";
            return;
        }

        int userId;
        if (!getUserId(userKey, userId))
        {
            qDebug() << "Non existing user";
            return;
        }

        std::string chatRoomName = getChatRoomTableName(chatRoomId.m_id);

        if (onlyLeave)
        {
            query.prepare("DELETE FROM " + chatRoomName + "_members WHERE userId = " + std::to_string(userId) + ";");
            query.exec();
        }
        else
        {
            query.prepare("DELETE FROM ChatRoomCatalogue WHERE chatRoomId = " + std::to_string(chatRoomId.m_id) + ";");
            query.exec();

            query.prepare("DROP TABLE " + chatRoomName + ";");
            query.exec();

            query.prepare("DROP TABLE " + chatRoomName + "_members;");
            query.exec();
        }
    }

    bool checkChatRoomOwner(ChatRoomId chatRoomId, const Key& userKey)
    {
        Query query(m_db);

        query.prepare("SELECT ownerPublicKey FROM ChatRoomCatalogue WHERE chatRoomId = " + std::to_string(chatRoomId.m_id) + ";");
        query.exec();

        if (!query.next())
        {
            qCritical() << "No chat room found";
            return false;
        }

        Key ownerKey;
        fromString(query.value(0).toString().toStdString(), ownerKey);
        return ownerKey == userKey;
    }

    void deleteMessage(ChatRoomId chatRoomId, MessageId messageId, Key userKey, std::function<void()> func) override
    {
        Query query(m_db);

        std::string chatRoomName = getChatRoomTableName(chatRoomId.m_id);
        int userId;
        getUserId(userKey, userId);

        query.prepare("DELETE FROM " + chatRoomName + " WHERE id = " + std::to_string(messageId.m_id) + " AND senderIdRef = " + std::to_string(userId) + ";");
        if (query.exec())
        {
            func();
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

IChatDatabase* createDatabase(IChatModel& chat)
{
    return (IChatDatabase*)new ChatDatabase(chat);
}
