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
    PacketHeader<HandshakeRequest>  m_handshakeRequest;
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
        if (m_clients.find(client.m_username) != m_clients.end())
        {
            return false;
        }
        m_clients[client.m_username] = client;
        return true;
    }

    bool removeClient(User client)
    {
        m_clients.erase(client.m_username);
        if (m_clients.empty())
        {
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
        qDebug() << "Packet received: " << packetType;
        switch(packetType)
        {
            case ConnectRequest::type:
            {
                qDebug() << "Connect request received";
                const ConnectRequest& request = *(reinterpret_cast<const ConnectRequest*>(readBuffer));

                //  Looking if user had any previous connection
                auto it = m_users.find(request.m_publicKey);
                if (it == m_users.end())
                {
                    auto userInfo = std::make_shared<UserInfo>(request.m_nickname, request.m_publicKey);
                    it = m_users.insert({request.m_publicKey, userInfo}).first;
                    it->second->m_publicKey = request.m_publicKey;
                    it->second->m_nickname = request.m_nickname;
                }

                // Add connection
                it->second->m_connections.emplace_back(session, request.m_deviceKey);

                // Send HandShake
                generateRandomKey(it->second->m_connections.back().m_handshakeRequest.m_packet.m_random);
                if (auto sessionPtr = session.lock(); sessionPtr)
                {
                    qDebug() << "Send packet";
                    sessionPtr->sendPacket(it->second->m_connections.back().m_handshakeRequest);
                }
                else{
                    qDebug() << "!sessionPtr";
                }
                break;
            }
            case HandshakeResponse::type:
            {
                const HandshakeResponse& response = *(reinterpret_cast<const HandshakeResponse*>(readBuffer));
                if (!response.verify())
                {
                    qDebug() << "Bad Handshake response";
                    if (auto sessionPtr = session.lock(); sessionPtr)
                    {
                        qDebug() << "Close connection";
                        sessionPtr->closeConnection();
                    }
                }

                // Find user
                auto it = m_users.find(response.m_publicKey);
                if (it == m_users.end())
                {
                    if (auto sessionPtr = session.lock(); sessionPtr)
                    {
                        qDebug() << "Close connection2";
                        sessionPtr->closeConnection();
                    }
                }

                // Find connection
                auto connectionIt = std::find_if(it->second->m_connections.begin(), it->second->m_connections.end(), [&](const auto& connection){
                   return connection.m_deviceKey == response.m_deviceKey;
                });
                if (connectionIt == it->second->m_connections.end())
                {
                    if (auto sessionPtr = session.lock(); sessionPtr)
                    {
                        qDebug() << "Close connection3";
                        sessionPtr->closeConnection();
                    }
                }
                qDebug() << "Successfull Handshake response";

                break;
            }
        }
    }

};
