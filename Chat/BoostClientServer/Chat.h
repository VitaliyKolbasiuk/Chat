#include <iostream>
#include <list>
#include <map>
#include <optional>

//#include "ChatInterfaces.h"
//#include "Protocol.h"
//#include "Utils.h"
#include "ServerSession.h"
#include "Types.h"

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
    std::map<ChatRoomId, ChatRoom> m_chatRooms;
    IChatDatabase& m_database;

public:
    Chat(IChatDatabase& database) : m_database(database) {}

    virtual void onPacketReceived(uint16_t packetType, const uint8_t* readBuffer, uint16_t packetSize, std::weak_ptr<ServerSession> session) override
    {
        qDebug() << "Packet received: " << packetType;
        switch(packetType)
        {
            case ConnectRequest::type:
            {
                qDebug() << "<ConnectRequest> received";
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
                if (const auto& sessionPtr = session.lock(); sessionPtr)
                {
                    sessionPtr->sendPacket(it->second->m_connections.back().m_handshakeRequest);
                }
                else{
                    qCritical() << "!sessionPtr";
                }
                break;
            }
            case HandshakeResponse::type:
            {
                qDebug() << "<HandshakeResponse> received";
                const HandshakeResponse& response = *(reinterpret_cast<const HandshakeResponse*>(readBuffer));
                if (!response.verify())
                {
                    qCritical() << "Bad Handshake response";
                    if (auto sessionPtr = session.lock(); sessionPtr)
                    {
                        qCritical() << "Close connection";
                        sessionPtr->closeConnection();
                    }
                    return;
                }

                // Find user
                const auto& it = m_users.find(response.m_publicKey);
                if (it == m_users.end())
                {
                    if (const auto& sessionPtr = session.lock(); sessionPtr)
                    {
                        qCritical() << "Close connection2";
                        sessionPtr->closeConnection();
                    }
                    return;
                }

                // Find connection
                const auto& connectionIt = std::find_if(it->second->m_connections.begin(), it->second->m_connections.end(), [&](const auto& connection){
                   return connection.m_deviceKey == response.m_deviceKey;
                });
                if (connectionIt == it->second->m_connections.end())
                {
                    if (const auto& sessionPtr = session.lock(); sessionPtr)
                    {
                        qCritical() << "Close connection3";
                        sessionPtr->closeConnection();
                    }
                    return;
                }

                // Security from malicious
                if (connectionIt->m_handshakeRequest.m_packet.m_random != response.m_random)
                {
                    if (const auto& sessionPtr = session.lock(); sessionPtr)
                    {
                        qCritical() << "Close connection4";
                        sessionPtr->closeConnection();
                    }
                    return;
                }

                qDebug() << "Successfull Handshake response";

                boost::asio::post( gDatabaseIoContext, [=, this]() mutable
                {
                    m_database.onUserConnected(response.m_publicKey, response.m_deviceKey, response.m_nickname, session);
                } );

//                if (const auto& sessionPtr = session.lock(); sessionPtr)
//                {
//                    sessionPtr->readPacket();
//                }
                break;
            }
            case RequestMessagesPacket::type:
            {
                qDebug() << "<RequestMessagesPacket> received";
                const RequestMessagesPacket& request = *(reinterpret_cast<const RequestMessagesPacket*>(readBuffer));
                qDebug() << "Request messages packet received: " << request.m_chatRoomId.m_id;
                boost::asio::post( gDatabaseIoContext, [=, this]() mutable
                {
                    //m_database.onUserConnected();
                } );
                break;
            }
            case CreateChatRoomPacket::type:
            {
                qDebug() << "<CreateChatRoomPacket> received";
                const CreateChatRoomPacket& packet = *(reinterpret_cast<const CreateChatRoomPacket*>(readBuffer));
                if (!packet.verify())
                {
                    qCritical() << "Bad packet sign";
                    break;
                }

//                boost::asio::post( gDatabaseIoContext, [=, this]() mutable
//                {
//                    m_database.createChatRoomTable();
//                } );

                break;
            }
        }
    }

};
