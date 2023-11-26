#include <iostream>
#include <list>
#include <map>
#include <optional>

//#include "ChatInterfaces.h"
//#include "Protocol.h"
//#include "Utils.h"
#include "ServerSession.h"

class ChatRoom;

struct User
{
    ChatRoom* m_room = nullptr;
    ServerSession* m_session;
    std::string m_username;

    User(ChatRoom* room = nullptr, ServerSession* session = nullptr) : m_room(room), m_session(session) {}

    User (const User&) = default;
    User& operator=(const User&) = default;
};

struct Connection
{
    std::weak_ptr<ServerSession>    m_session;
    Key                             m_deviceKey;
    RequestHeader<HandShakeRequest> m_handShakeRequest;
};

struct UserInfo
{
    std::string             m_nickname;
    Key                     m_publicKey;
    std::vector<Connection> m_connections;

    UserInfo(const std::string& nickname, const Key& publicKey ) : m_nickname(nickname), m_publicKey(publicKey) {}
};

class ChatRoom
{
    std::map<std::string, User> m_clients;
public:
    //std::map<std::string, User> m_clients;

    ChatRoom() {}

    bool addClient(User client)
    {
        if (m_clients.find(client.m_username) != m_clients.end()){
            return false;
        }
        m_clients[client.m_username] = client;
        return true;
    }

    bool removeClient(User client)
    {
        m_clients.erase(client.m_username);
        if (m_clients.empty()){
            return false;
        }
        return true;
    }

    std::map<std::string, User>& clients()
    {
        return m_clients;
    }

};

class Chat: public IChat
{
    std::map<Key, std::shared_ptr<UserInfo>> m_users;
    std::map<std::string, ChatRoom> m_chatRooms;
    IChatDatabase& m_database;

public:
    Chat(IChatDatabase& database) : m_database(database) {}

    void updateTable(const std::string &chatRoomName, const std::string &username)
    {
        std::shared_ptr<boost::asio::streambuf> wrStreambuf = std::make_shared<boost::asio::streambuf>();
        std::ostream os(&(*wrStreambuf));
        for (const auto &user : m_chatRooms[chatRoomName].clients())
        {
            os << UPDATE_THE_USER_TABLE_CMD";" + username + ";\n";
            //user.second.m_session->sendMessage(wrStreambuf);
        }
    }

    virtual void onPacketReceived(uint16_t packetType, const uint8_t* readBuffer, uint16_t packetSize, std::weak_ptr<ServerSession> session) override
    {

        switch(packetType)
        {
            case ConnectRequest::type:
            {
                const ConnectRequest& request = *(reinterpret_cast<const ConnectRequest*>(readBuffer));

                auto it = m_users.find(request.m_publicKey);
                if (it == m_users.end())
                {
                    it = m_users.insert({request.m_publicKey, std::make_shared<UserInfo>(request.m_nickname, request.m_publicKey)}).first;
                    it->second->m_publicKey = request.m_publicKey;
                    it->second->m_nickname = request.m_nickname;
                }
                it->second->m_connections.emplace_back(session, request.m_deviceKey);

                generateRandomKey(it->second->m_connections.back().m_handShakeRequest.m_request.m_random);
                if (auto sessionPtr = session.lock(); sessionPtr)
                {
                    sessionPtr->sendPacket(it->second->m_connections.back().m_handShakeRequest);
                }
            }

        }
    }

};
