#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <boost/tokenizer.hpp>
#include <QObject>
#include <ctime>

#include "ClientInterfaces.h"
#include "TcpClient.h"
//#include "../Log.h"

#include "Settings.h"
#include "ed25519/src/ed25519.h"


struct ModelChatRoomInfo{
    ChatRoomId  m_id;
    std::string m_name;
    int         m_position = 0;
    int         m_offset = 0;

    ModelChatRoomInfo() = default;
    ModelChatRoomInfo(const ModelChatRoomInfo&) = default;
    ModelChatRoomInfo(const ChatRoomId& id,const std::string& name) : m_id(id), m_name(name) {}

    struct Record{
        std::time_t  m_time;
        UserId       m_userId;
        std::string  m_username;
        std::string  m_text;
    };
    std::vector<Record> m_records;
};

using ModelChatRoomList = std::map<ChatRoomId, ModelChatRoomInfo>;

class QChatClient : public QObject, public IChatClient
{
   Q_OBJECT

public:

signals:
    void OnMessageReceived(ChatRoomId chatRoomId, const std::string& username, const std::string& message, uint64_t time);

    void OnTableChanged(const ModelChatRoomList& chatRoomInfoList);

    void OnChatRoomAddedOrDeleted(const ChatRoomId& chatRoomId, const std::string chatRoomName, bool isAdd);

private:
    std::weak_ptr<TcpClient>  m_tcpClient;
    std::string m_chatClientName;
    ModelChatRoomList m_chatRoomInfoList;
    Settings& m_settings;

private:
    PacketHeader<ConnectRequest> m_connectRequest;


public:
    QChatClient(Settings& settings) : m_settings(settings) {

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

        if (const auto& tcpClient = m_tcpClient.lock(); tcpClient )
        {
            qDebug() << "Started read loop";
            tcpClient->readPacket();        //readPacket() will always call itself
        }

        if (const auto& tcpClient = m_tcpClient.lock(); tcpClient )
        {
            qDebug() << "Connect request has been sent";
            tcpClient->sendPacket(m_connectRequest);
        }
    }

    virtual void onPacketReceived ( uint16_t packetType, uint8_t* packet, uint16_t packetSize) override
    {
        qDebug() << "Received packet type: " << packetType;
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
                qDebug() << "ChatRoomListPacket received";
                ChatRoomInfoList chatRoomList = parseChatRoomList(packet, packetSize - sizeof(PacketHeaderBase));

                for (const auto& chatRoomInfo : chatRoomList)
                {
                    m_chatRoomInfoList[ChatRoomId(chatRoomInfo.m_chatRoomId)] = ModelChatRoomInfo{ChatRoomId(chatRoomInfo.m_chatRoomId), chatRoomInfo.m_chatRoomName};
                }

                // UPDATE ChatRoomList
                emit OnTableChanged(m_chatRoomInfoList);

                for (const auto& [key, chatRoomInfo] : m_chatRoomInfoList)
                {
                    PacketHeader<RequestMessagesPacket> request;
                    request.m_packet.m_chatRoomId = chatRoomInfo.m_id;
                    request.m_packet.m_messageNumber = 100;
                    if (const auto& tcpClient = m_tcpClient.lock(); tcpClient )
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
                    m_chatRoomInfoList[response.m_chatRoomId] = ModelChatRoomInfo{response.m_chatRoomId, response.m_chatRoomName};
                }
                else
                {
                    m_chatRoomInfoList.erase(response.m_chatRoomId);
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

                emit OnMessageReceived(chatRoomId, username, message, time);    // TODO
            }
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

        if (const auto& tcpClient = m_tcpClient.lock(); tcpClient )
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

        if (const auto& tcpClient = m_tcpClient.lock(); tcpClient )
        {
            tcpClient->sendPacket(packet);
        }
        return true;
    }

    virtual void closeConnection() override
    {
        exit(1);
    }
};
