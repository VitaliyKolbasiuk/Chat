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
    std::string             m_username;

    UserInfo() {}
};

struct ChatRoom
{
    std::string chatRoomName;
    std::string chatRoomTableName;
    Key         ownerPublicKey;
    bool        isPrivate;
    std::map<Key, std::weak_ptr<UserInfo>> m_clients;

};

class ChatModel: public IChatModel
{
    // TODO remove user when disconnects
    std::map<Key, std::shared_ptr<UserInfo>> m_users;
    std::map<ChatRoomId, ChatRoom> m_chatRooms;
    IChatDatabase& m_database;

public:
    ChatModel() : m_database(*createDatabase(*this))
    {
        qDebug() << "Read ChatRoomCatalogue";
        m_database.readChatRoomCatalogue([this](uint32_t chatRoomId, const std::string& chatRoomName, const std::string& chatRoomTableName, const Key& publicKey, bool isPrivate){
            m_chatRooms[ChatRoomId(chatRoomId)] = ChatRoom{chatRoomName, chatRoomTableName, publicKey, isPrivate};
        });
    }

    ~ChatModel()
    {
        delete &m_database;
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
                    userInfo->m_username = request.m_nickname;
                    it = m_users.insert({request.m_publicKey, userInfo}).first;
                    // PUSH BACK LOWER userInfo->m_connections.push_back(session);
                }

                // Add connection
                it->second->m_connections.emplace_back(session, request.m_deviceKey);

                // Send HandShake
                generateRandomKey(it->second->m_connections.back().m_handshakeRequest.m_packet.m_random);
                if (const auto sessionPtr = session.lock(); sessionPtr)
                {
                    sessionPtr->m_userKey = request.m_publicKey;
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
                    if (const auto sessionPtr = session.lock(); sessionPtr)
                    {
                        qCritical() << "Close connection3";
                        sessionPtr->closeConnection();
                    }
                    return;
                }

                // Security from malicious
                if (connectionIt->m_handshakeRequest.m_packet.m_random != response.m_random)
                {
                    if (const auto sessionPtr = session.lock(); sessionPtr)
                    {
                        qCritical() << "Close connection4";
                        sessionPtr->closeConnection();
                    }
                    return;
                }

                qDebug() << "Successfull Handshake response";

                boost::asio::post( gDatabaseIoContext, [=, this]() mutable
                {
                    m_database.onUserConnected(response.m_publicKey,
                                               response.m_deviceKey,
                                               response.m_nickname,
                           [this, response, session](const ChatRoomInfoList& chatRoomList)
                           {
                                if ( chatRoomList.empty())
                                    return;
                               boost::asio::post(gServerIoContext, [=, this]() mutable
                               {
                                   if (const auto sessionPtr = session.lock(); sessionPtr)
                                   {
                                       qDebug() << "ChatRoomListPacket has been sent";
                                       ChatRoomListPacket *packet = createChatRoomList(
                                               chatRoomList);

                                       sessionPtr->sendBufferedPacket<ChatRoomListPacket>(packet);

                                       for (auto &chatRoomInfo: chatRoomList)
                                       {
                                           m_chatRooms[(ChatRoomId) chatRoomInfo.m_chatRoomId].m_clients[response.m_publicKey] = m_users[response.m_publicKey];
                                       }
                                   }
                               });
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
                    m_database.onRequestMessages(request.m_chatRoomId, request.m_messageNumber, request.m_messageId, [=, this](const std::vector<ChatRoomRecord>& recordsList){
                        boost::asio::post(gServerIoContext, [=, this]() mutable
                        {
                            if (const auto sessionPtr = session.lock(); sessionPtr)
                            {
                                auto* packet = createChatRoomRecordPacket(request.m_chatRoomId, recordsList);
                                sessionPtr->sendBufferedPacket<ChatRoomRecordPacket>(packet);
                            }
                        });
                    });
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
                    uint32_t chatRoomId = (uint32_t)m_database.createChatRoomTable(packet.m_chatRoomName,
                                                                                   packet.m_isPrivate,
                                                                                   packet.m_publicKey,
                                                                                   session);

                    assert(m_users.find(packet.m_publicKey) != m_users.end());
                    m_chatRooms[static_cast<ChatRoomId>(chatRoomId)].m_clients[packet.m_publicKey] = m_users[packet.m_publicKey];
                    m_chatRooms[static_cast<ChatRoomId>(chatRoomId)].chatRoomName = packet.m_chatRoomName;
                    m_chatRooms[static_cast<ChatRoomId>(chatRoomId)].ownerPublicKey = packet.m_publicKey;
                } );
                break;
            }
            case SendTextMessagePacket::type:
            {
                uint64_t time;
                ChatRoomId chatRoomId;
                Key publicKey;
                std::string message = parseSendTextMessagePacket(readBuffer, packetSize ,time, chatRoomId, publicKey);

                boost::asio::post(gDatabaseIoContext, [=, this]() mutable{
                    int senderId;
                    if (auto messageId = m_database.appendMessageToChatRoom(chatRoomId.m_id, publicKey, time, message, senderId);
                        messageId != std::numeric_limits<uint32_t>::max())
                    {
                        boost::asio::post(gServerIoContext, [=, this]() mutable{
                            std::string username = m_users[publicKey]->m_username;
                            sendMessageToClients(message, (MessageId)messageId, chatRoomId, time, senderId, username);
                        });
                    }
                    else
                    {
                        qWarning() << "!!!Message wasn't added to database";
                    }
                });

                break;
            }
            case ConnectChatRoom::type:
            {
                const ConnectChatRoom& packet = *(reinterpret_cast<const ConnectChatRoom*>(readBuffer));


                if (auto sessionPtr = session.lock(); sessionPtr)
                {
                    boost::asio::post( gDatabaseIoContext, [=, this, userKey = sessionPtr->m_userKey]() mutable
                    {
                        m_database.onConnectToChatRoomMessage(packet.m_chatRoomName, userKey, session);
                    } );
                }
                break;
            }
            case SendDeleteChatRoomRequest::type:
            {
                const SendDeleteChatRoomRequest packet = *(reinterpret_cast<const SendDeleteChatRoomRequest*>(readBuffer));

                if (auto sessionPtr = session.lock(); sessionPtr)
                {
                    boost::asio::post(gDatabaseIoContext, [=, this, userKey = sessionPtr->m_userKey]() mutable
                    {
                        m_database.leaveChatRoom(packet.m_chatRoomId, userKey, packet.m_onlyLeave);
                    });
                }

                if (packet.m_onlyLeave)
                {
                    leaveChatRoom(packet.m_chatRoomId, session);
                }
                else
                {
                    deleteChatRoom(packet.m_chatRoomId);
                }

                break;
            }
        }
    }

    void sendMessageToClients(const std::string& message, MessageId messageId, ChatRoomId chatRoomId, uint64_t time, int senderId, const std::string& username)
    {
        auto it = m_chatRooms.find(chatRoomId);
        if (it == m_chatRooms.end())
        {
            qCritical() << "Internal error";
            return;
        }

        for (auto& user : it->second.m_clients)
        {
            auto userIt = m_users.find(user.first);
            if (userIt != m_users.end())
            {
                for (auto& connection : userIt->second->m_connections)
                {
                    if (auto sessionPtr = connection.m_session.lock(); sessionPtr)
                    {
                        auto* packet = createTextMessagePacket(message, messageId, chatRoomId, senderId, username, time);
                        sessionPtr->sendBufferedPacket<TextMessagePacket>(packet);
                    }
                }
            }
        }
    }

    virtual void updateChatRoomList(const std::string& chatRoomName, uint32_t id, bool isAdd, bool isOwner, std::weak_ptr<ServerSession> session) override
    {
        PacketHeader<ChatRoomUpdatePacket>* packet = new PacketHeader<ChatRoomUpdatePacket>;
        std::memcpy(&packet->m_packet.m_chatRoomName, chatRoomName.c_str(), chatRoomName.size() + 1);
        packet->m_packet.m_chatRoomId = id;
        packet->m_packet.m_addOrDelete = isAdd;
        packet->m_packet.m_isOwner = isOwner;
        if (const auto sessionPtr = session.lock(); sessionPtr)
        {
            if (isAdd)
            {
                m_chatRooms[(ChatRoomId)id].m_clients[sessionPtr->m_userKey] = m_users[sessionPtr->m_userKey];
            }
            else
            {
                if (auto userIt =  m_chatRooms[(ChatRoomId)id].m_clients.find(sessionPtr->m_userKey);
                        userIt !=  m_chatRooms[(ChatRoomId)id].m_clients.end())
                {
                    m_chatRooms[(ChatRoomId)id].m_clients.erase(userIt);
                }
            }
        }
        boost::asio::post(gServerIoContext, [packet, session]{
            if (const auto sessionPtr = session.lock(); sessionPtr)
            {
                sessionPtr->sendPacket(packet);
            }
        });
    }

    void connectToChatRoomFailed(const std::string& chatRoomName, std::weak_ptr<ServerSession> session) override
    {
        PacketHeader<ConnectChatRoomFailed>* packet = new PacketHeader<ConnectChatRoomFailed>;
        std::memcpy(&packet->m_packet.m_chatRoomName, chatRoomName.c_str(), chatRoomName.size() + 1);

        boost::asio::post(gServerIoContext, [packet, session]{
            if (const auto sessionPtr = session.lock(); sessionPtr)
            {
                sessionPtr->sendPacket(packet);
            }
        });
    }

    void deleteChatRoom(ChatRoomId chatRoomId)
    {
        PacketHeader<ChatRoomUpdatePacket> packet;
        packet.m_packet.m_chatRoomId = chatRoomId;
        std::string chatRoomName = m_chatRooms[chatRoomId].chatRoomName;
        std::memcpy(&packet.m_packet.m_chatRoomName, chatRoomName.c_str(), chatRoomName.size() + 1) ;
        packet.m_packet.m_addOrDelete = false;

        auto clientsIt = m_chatRooms[chatRoomId].m_clients;
        for (auto& [userKey, userInfo] : clientsIt)
        {
            auto userInfoPtr = userInfo.lock();
            if (!userInfoPtr)
            {
                qCritical() << "Internal error";
                return;
            }
            for (auto& connection : userInfoPtr->m_connections)
            {
                if (auto sessionPtr = connection.m_session.lock(); sessionPtr)
                {
                    sessionPtr->sendPacket(packet);
                }
            }
        }

        m_chatRooms.erase(chatRoomId);
    }

    void leaveChatRoom(ChatRoomId chatRoomId, std::weak_ptr<ServerSession> session)
    {
        PacketHeader<ChatRoomUpdatePacket> packet;
        packet.m_packet.m_chatRoomId = chatRoomId;
        std::string chatRoomName = m_chatRooms[chatRoomId].chatRoomName;
        std::memcpy(&packet.m_packet.m_chatRoomName, chatRoomName.c_str(), chatRoomName.size() + 1) ;
        packet.m_packet.m_addOrDelete = false;

        if (auto sessionPtr = session.lock(); sessionPtr)
        {
            sessionPtr->sendPacket(packet);
            m_chatRooms[chatRoomId].m_clients.erase(sessionPtr->m_userKey);
        }
    }

    void closeConnection(ServerSession& serverSession) override
    {
        for(auto& [chatRoomId, chatRoom] : m_chatRooms)
        {
            if (auto clientIt = chatRoom.m_clients.find(serverSession.m_userKey); clientIt != chatRoom.m_clients.end())
            {
                auto userInfoPtr = clientIt->second.lock();
                if (!userInfoPtr)
                {
                    qCritical() << "Internal error";
                    chatRoom.m_clients.erase(clientIt);
                    return;
                }
                std::erase_if(userInfoPtr->m_connections, [&serverSession](const auto& connection){
                    if (auto sessionPtr = connection.m_session.lock(); sessionPtr)
                    {
                        return sessionPtr->m_socket.remote_endpoint() == serverSession.m_socket.remote_endpoint();
                    }
                    return false;
                });
            }
        }
        if (auto userInfoIt = m_users.find(serverSession.m_userKey); userInfoIt != m_users.end())
        {
            if (userInfoIt->second->m_connections.empty())
            {
                m_users.erase(userInfoIt);
            }
        }
    }
};
