#pragma once

#include <QTimer>

#include "TcpClient.h"
#include "Settings.h"

struct ChatRoomData{
    ChatRoomId  m_id;
    std::string m_name;
    bool        m_isOwner;
    int         m_position = 0;
    int         m_offset = 0;

    ChatRoomData() = default;
    ChatRoomData(const ChatRoomData&) = default;
    ChatRoomData(const ChatRoomId& id ,const std::string& name, bool isOwner) : m_id(id), m_name(name), m_isOwner(isOwner) {}

    struct Record{
        MessageId    m_messageId;
        std::time_t  m_time;
        UserId       m_userId;
        std::string  m_username;
        std::string  m_text;

        bool operator< (const Record& record) const
        {
            return m_time < record.m_time;
        }
    };

    std::map<int, Record> m_records;

    void addRecords(const std::vector<ChatRoomRecord>& records)
    {
        for (const auto& record : records)
        {
            m_records[record.m_messageId.m_id] = {record.m_messageId, record.m_time, record.m_userId, record.m_username, record.m_text};
        }
    }

    void addMessage(const ChatRoomRecord& record)
    {
        m_records[record.m_messageId.m_id] = {record.m_messageId, record.m_time, record.m_userId, {}, record.m_text};
    }
};

using ChatRoomMap = std::map<ChatRoomId, ChatRoomData>;

class ChatClient : public QObject, public IChatClient
{
   Q_OBJECT

public:

signals:
    void OnMessageReceived(ChatRoomId chatRoomId, MessageId messageId, const std::string& username, const std::string& message, uint64_t time);

    void OnTableChanged(const ChatRoomMap& chatRoomInfoList);

    void OnChatRoomAddedOrDeleted(const ChatRoomId& chatRoomId, const std::string chatRoomName, bool isAdd);

    void updateChatRoomRecords(ChatRoomId chatRoomId);

    void onMessageDeleted(ChatRoomId chatRoomId, MessageId messageId);

    void onMessageEdited(ChatRoomId chatRoomId, MessageId messageId, const std::string& username, const std::string& message);

private:
    std::weak_ptr<TcpClient>  m_tcpClient;
    std::string m_chatClientName;
    ChatRoomMap m_chatRoomMap;
    Settings& m_settings;

private:
    PacketHeader<ConnectRequest> m_connectRequest;


public:
    ChatClient(Settings& settings) : m_settings(settings) {

    }

    void setTcpClient(const std::weak_ptr<TcpClient>& tcpClient)
    {
        m_tcpClient = tcpClient;
    }

    virtual void onSocketConnected() override
    {

        m_connectRequest.m_packet.m_deviceKey = m_settings.m_deviceKey;
        m_connectRequest.m_packet.m_publicKey = m_settings.m_keyPair.m_publicKey;

        size_t maxSize = sizeof(m_connectRequest.m_packet.m_nickname) - 1;
        if (m_settings.m_username.size() > maxSize)
        {
            m_settings.m_username.erase(m_settings.m_username.begin() + maxSize, m_settings.m_username.end());
        }
        std::memcpy(&m_connectRequest.m_packet.m_nickname, m_settings.m_username.c_str(), m_settings.m_username.size() + 1);

        if (const auto tcpClient = m_tcpClient.lock(); tcpClient )
        {
            qDebug() << "Started read loop";
            tcpClient->readPacket();        //readPacket() will always call itself
        }

        if (const auto tcpClient = m_tcpClient.lock(); tcpClient )
        {
            qDebug() << "Connect request has been sent";
            tcpClient->sendPacket(m_connectRequest);
        }
    }

    virtual void onPacketReceived ( uint16_t packetType, uint8_t* packet, uint16_t packetSize) override
    {
        qDebug() << "Received packet packetType: " << packetType;
        switch(packetType)
        {
            case HandshakeRequest::type:
            {
                qDebug() << "Handshake received";
                HandshakeRequest request{};
                assert(packetSize == sizeof(request));
                std::memcpy(&request, packet, packetSize);
                onHandshake(request);
                break;
            }
            case ChatRoomListPacket::type:
            {
                ChatRoomInfoList chatRoomList = parseChatRoomList(packet, packetSize);

                for (const auto& chatRoomInfo : chatRoomList)
                {
                    m_chatRoomMap[ChatRoomId(chatRoomInfo.m_chatRoomId)] = ChatRoomData{ChatRoomId(chatRoomInfo.m_chatRoomId),
                                                                                        chatRoomInfo.m_chatRoomName,
                                                                                        chatRoomInfo.m_ownerPublicKey == m_settings.m_keyPair.m_publicKey};
                }

                // UPDATE ChatRoomList
                emit OnTableChanged(m_chatRoomMap);

                for (const auto& [key, chatRoomInfo] : m_chatRoomMap)
                {
                    PacketHeader<RequestMessagesPacket> request;
                    request.m_packet.m_chatRoomId = chatRoomInfo.m_id;
                    request.m_packet.m_messageNumber = 100;
                    if (const auto tcpClient = m_tcpClient.lock(); tcpClient )
                    {
                        tcpClient->sendPacket(request);
                    }
                }

                break;
            }
            case ChatRoomUpdatePacket::type:
            {
                const ChatRoomUpdatePacket& response = *(reinterpret_cast<const ChatRoomUpdatePacket*>(packet));

                if (response.m_addOrDelete)
                {
                    m_chatRoomMap[response.m_chatRoomId] = ChatRoomData{response.m_chatRoomId,
                                                                        response.m_chatRoomName,
                                                                        response.m_isOwner};
                }
                else
                {
                    m_chatRoomMap.erase(response.m_chatRoomId);
                }

                emit OnChatRoomAddedOrDeleted(response.m_chatRoomId, response.m_chatRoomName, response.m_addOrDelete);

                break;
            }
            case TextMessagePacket::type:
            {
                uint64_t time;
                ChatRoomId chatRoomId;
                std::string username;
                UserId userId;
                MessageId messageId;
                std::string message = parseTextMessagePacket(packet, packetSize ,time, chatRoomId, messageId, userId, username);

                m_chatRoomMap[chatRoomId].addMessage({messageId, (std::time_t)time, userId, message});
                emit OnMessageReceived(chatRoomId, messageId, username, message, time);
                break;
            }
            case ChatRoomRecordPacket::type:
            {
                ChatRoomId chatRoomId;
                std::vector<ChatRoomRecord> records = parseChatRoomRecordPacket(packet, packetSize, chatRoomId);
                QTimer::singleShot(0, this, [=, this](){
                    m_chatRoomMap[chatRoomId].addRecords(records);
                    emit updateChatRoomRecords(chatRoomId);
                });

                break;
            }
            case ConnectChatRoomFailed::type:
            {

                break;
            }
            case DeleteMessageResponse::type:
            {
                const DeleteMessageResponse response = *(reinterpret_cast<const DeleteMessageResponse*>(packet));

                if (response.m_isDeleted)
                {
                    m_chatRoomMap[response.m_chatRoomId].m_records.erase(response.m_messageId.m_id);
                    emit onMessageDeleted(response.m_chatRoomId, response.m_messageId);
                }

                break;
            }
            case EditMessageResponse::type:
            {
                ChatRoomId  chatRoomId;
                MessageId   messageId;
                std::string username;
                std::string editedMessage = parseEditMessageResponsePacket(packet, packetSize, chatRoomId, messageId, username);

                emit onMessageEdited(chatRoomId, messageId, username, editedMessage);

                m_chatRoomMap[chatRoomId].m_records[messageId.m_id].m_username = username;
                m_chatRoomMap[chatRoomId].m_records[messageId.m_id].m_text = editedMessage;
            }
        }
    }

    void deleteMessage(ChatRoomId chatRoomId, MessageId messageId)
    {
        PacketHeader<DeleteMessageRequest> packet;
        packet.m_packet.m_chatRoomId = chatRoomId;
        packet.m_packet.m_messageId  = messageId;

        if (auto tcpClient = m_tcpClient.lock(); tcpClient)
        {
            tcpClient->sendPacket(packet);
        }
    }

    void onHandshake(const HandshakeRequest& request)
    {
        PacketHeader<HandshakeResponse> response;
        response.m_packet.m_publicKey = m_settings.m_keyPair.m_publicKey;
        assert(m_settings.m_username.size() < sizeof(response.m_packet.m_nickname));
        std::memcpy(&response.m_packet.m_nickname, &m_settings.m_username[0], m_settings.m_username.size() + 1);
        response.m_packet.m_deviceKey = m_settings.m_deviceKey;
        response.m_packet.m_random = request.m_random;
        response.m_packet.sign(m_settings.m_keyPair.m_privateKey);

        if (const auto tcpClient = m_tcpClient.lock(); tcpClient )
        {
            tcpClient->sendPacket(response);
        }
    }

    bool createChatRoom(const std::string& name, bool isPrivate)
    {
        PacketHeader<CreateChatRoomPacket> packet;
        if (name.size() + 1 > sizeof(packet.m_packet.m_chatRoomName))
        {
            return false;
        }
        std::memcpy(&packet.m_packet.m_chatRoomName, &name[0], name.size() + 1);
        packet.m_packet.m_isPrivate = isPrivate;
        packet.m_packet.m_publicKey = m_settings.m_keyPair.m_publicKey;
        packet.m_packet.sign(m_settings.m_keyPair.m_privateKey);

        if (const auto tcpClient = m_tcpClient.lock(); tcpClient )
        {
            tcpClient->sendPacket(packet);
        }
        return true;
    }

    bool connectToChatRoom(const std::string& chatRoomName)
    {
        PacketHeader<ConnectChatRoom> packet;
        if (chatRoomName.size() + 1 > sizeof(packet.m_packet.m_chatRoomName))
        {
            return false;
        }
        std::memcpy(&packet.m_packet.m_chatRoomName, chatRoomName.c_str(), chatRoomName.size() + 1);


        if (const auto tcpClient = m_tcpClient.lock(); tcpClient)
        {
            tcpClient->sendPacket(packet);
        }
        return true;
    }

    void sendDeleteChatRoomRequest(ChatRoomId chatRoomId, bool onlyLeave)
    {
        PacketHeader<SendDeleteChatRoomRequest> packet;
        packet.m_packet.m_chatRoomId = chatRoomId;
        packet.m_packet.m_onlyLeave  = onlyLeave;

        if (auto tcpClient = m_tcpClient.lock(); tcpClient)
        {
            tcpClient->sendPacket(packet);
        }
    }

    void changeUsername(const std::string& newUsername)
    {
        PacketHeader<ChangeUsernameRequest> packet;
        std::memcpy(packet.m_packet.m_newUsername, newUsername.c_str(), newUsername.size() + 1);

        if (auto tcpClient = m_tcpClient.lock(); tcpClient)
        {
            tcpClient->sendPacket(packet);
        }
    }

    ChatRoomMap& getChatRoomMap()
    {
        return m_chatRoomMap;
    }

    virtual void closeConnection() override
    {
        exit(1);
    }
};
