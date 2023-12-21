#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <boost/tokenizer.hpp>
#include <QObject>

#include "ClientInterfaces.h"
#include "TcpClient.h"
//#include "../Log.h"

#include "Settings.h"
#include "ed25519/src/ed25519.h"


class QChatClient : public QObject, public IChatClient
{
   Q_OBJECT

public:

signals:
    void OnMessageReceived(QString username, QString message);

    void OnTableChanged(QString username);

private:
    std::weak_ptr<TcpClient>  m_tcpClient;
    std::string m_chatClientName;
    ChatRoomInfoList m_chatRoomInfoList;
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
        std::memcpy(&m_connectRequest.m_packet.m_nickname, m_settings.m_username.c_str(), m_settings.m_username.size());

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
                HandshakeRequest request;
                assert(packetSize == sizeof(request));
                std::memcpy(&request, packet, packetSize);
                onHandshake(request);
                break;
            }
            case ChatRoomListPacket::type:
            {
                qDebug() << "ChatRoomListPacket received";
                ChatRoomInfoList chatRoomList = parseChatRoomList(packet, packetSize - sizeof(PacketHeaderBase));
                m_chatRoomInfoList = chatRoomList;
                // UPDATE ChatRoomList

                break;
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
            qDebug() << "Handshake response has been sent";
            tcpClient->sendPacket(response);
        }
    }



    virtual void handleServerMessage(const std::string& command, boost::asio::streambuf& message) override
    {
        if ( message.size() <= 0 )
        {
            return;
        }

        std::istringstream input;
        input.str(std::string((const char*)message.data().data(), message.size()));

        if (command == MESSAGE_FROM_CMD)
        {
            std::string username;
            std::string message;

            std::getline(input, message, ';');
            std::getline(input, username, ';');

            if (username != m_chatClientName)
            {
                emit OnMessageReceived(QString::fromStdString("<HTML> <font color=\"yellow\">" + username + " </font>" ), QString::fromStdString(message));
            }
            else
            {
                emit OnMessageReceived(QString::fromStdString("<HTML> <font color=\"green\">" + username + ": </font>"), QString::fromStdString(message));
            }
        }
        else if (command == UPDATE_THE_USER_TABLE_CMD)
        {
            std::string username;
            std::getline(input, username, ';');
            emit OnTableChanged(QString::fromStdString(username));
        }
    }
    virtual bool sendUserMessage(const std::string& message) override
    {
        std::shared_ptr<boost::asio::streambuf> wrStreambuf = std::make_shared<boost::asio::streambuf>();
        std::ostream os(&(*wrStreambuf));
        //os << message + m_chatRoomName + ";" + m_chatClientName + ";\n";
        if (const auto& tcpClient = m_tcpClient.lock(); tcpClient )
        {
            //tcpClient->sendMessageToServer(wrStreambuf);
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual void closeConnection() override
    {
        sendUserMessage(EXIT_THE_CHAT_CMD ";");
        exit(1);
    }
};
