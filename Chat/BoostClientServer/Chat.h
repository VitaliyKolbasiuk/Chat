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

struct Connection
{
    std::weak_ptr<ServerSession>    m_session;
    Key                             m_deviceKey;
    PacketHeader<HandshakeRequest>  m_handshakeRequest;
};

struct UserInfo
{
    std::vector<Connection> m_connections;

    UserInfo() {}
};

struct ChatRoom
{
    std::string chatRoomName;
    std::string chatRoomTableName;
    Key         ownerPublicKey;
    bool        isPrivate;
    std::map<Key, std::weak_ptr<UserInfo>> m_clients;


    std::map<Key, std::weak_ptr<UserInfo>>& clients()
    {
        return m_clients;
    }

};

class Chat: public IChat
{
    // TODO remove user when disconnects
    std::map<Key, std::shared_ptr<UserInfo>> m_users;
    std::map<ChatRoomId, ChatRoom> m_chatRooms;
    IChatDatabase& m_database;

public:
    Chat(IChatDatabase& database) : m_database(database)
    {
        m_database.readChatRoomCatalogue([this](uint32_t chatRoomId, const std::string& chatRoomName, const std::string& chatRoomTableName, const Key& publicKey, bool isPrivate){
            m_chatRooms[ChatRoomId(chatRoomId)] = ChatRoom{chatRoomName, chatRoomTableName, publicKey, isPrivate};
        });
    }

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
                    auto userInfo = std::make_shared<UserInfo>();
                    it = m_users.insert({request.m_publicKey, userInfo}).first;
                    // PUSH BACK LOWER userInfo->m_connections.push_back(session);
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
                    m_database.onUserConnected(response.m_publicKey, response.m_deviceKey, response.m_nickname, [this, response, session](const ChatRoomInfoList& chatRoomList){
                        if (const auto &sessionPtr = session.lock(); sessionPtr)
                        {
                            qDebug() << "ChatRoomListPacket has been sent";
                            ChatRoomListPacket *packet = createChatRoomList(chatRoomList);
                            sessionPtr->sendBufferedPacket<ChatRoomListPacket>(packet);

                            for (auto& chatRoomInfo : chatRoomList)
                            {
                                m_chatRooms[(ChatRoomId)chatRoomInfo.m_chatRoomId].m_clients[response.m_publicKey] = m_users[response.m_publicKey];
                            }
                        }
                    });
                } );

                break;
            }
            case RequestMessagesPacket::type:
            {
                qDebug() << "<RequestMessagesPacket> received";
                const RequestMessagesPacket& request = *(reinterpret_cast<const RequestMessagesPacket*>(readBuffer));
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

                boost::asio::post( gDatabaseIoContext, [=, this]() mutable
                {
                    ChatRoomId chatRoomId {(uint32_t)m_database.createChatRoomTable(packet.m_chatRoomName, packet.m_isPrivate, packet.m_publicKey, session)};
                    assert(m_users.find(packet.m_publicKey) != m_users.end());
                    m_chatRooms[chatRoomId].m_clients[packet.m_publicKey] = m_users[packet.m_publicKey];
                } );
                break;
            }
            case TextMessagePacket::type:
            {
                const TextMessagePacket& packet = *(reinterpret_cast<const TextMessagePacket*>(readBuffer));
                uint64_t time;
                ChatRoomId chatRoomId;
                Key publicKey;
                std::string message = parseTextMessagePacket(readBuffer, packetSize ,time, chatRoomId, publicKey);

                boost::asio::post(gDatabaseIoContext, [=, this]() mutable{
                    int senderId;
                    if (m_database.appendMessageToChatRoom(chatRoomId.m_id, publicKey, time, message, senderId))
                    {
                        boost::asio::post(gServerIoContext, [=, this]() mutable{
                            sendMessageToClients(message, chatRoomId, time, senderId, publicKey);
                        });
                    }
                    else
                    {

                    }
                });

                break;
            }
        }
    }

    void sendMessageToClients(const std::string& message, ChatRoomId chatRoomId, uint64_t time, int senderId, const Key& publicKey)
    {
        auto it = m_chatRooms.find(chatRoomId);
        if (it == m_chatRooms.end())
        {
            qCritical() << "Internal error";
            return;
        }
        for (auto& user : it->second.clients())
        {
            auto userIt = m_users.find(user.first);
            if (userIt != m_users.end())
            {
                for (auto& connection : userIt->second->m_connections)
                {
                    if (auto session = connection.m_session.lock(); session)
                    {
                        auto* packet = createTextMessagePacket(message, chatRoomId, publicKey, &time);
                        session->sendBufferedPacket<TextMessagePacket>(packet);
                    }
                }
            }
        }
    }
};
