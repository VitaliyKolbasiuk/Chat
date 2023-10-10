#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <boost/tokenizer.hpp>

#include "ClientInterfaces.h"
#include "TcpClient.h"
//#include "../Log.h"

class ChatClient : public IChatClient
{
    TcpClient* m_tcpClient = nullptr;
    std::string m_chatClientName;
    std::string m_chatRoomName;

public:
    ChatClient() {}

    void setTcpClient(TcpClient* tcpClient)
    {
        m_tcpClient = tcpClient;
    }

    virtual void saveClientInfo(const std::string& chatClientName, const std::string& chatRoomName) override
    {
        m_chatClientName = chatClientName;
        m_chatRoomName = chatRoomName;
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
                std::cout << '[' << username << "]: " << message << std::endl;
            }
            else
            {
                std::cout << "-->[" << username << "]: " << message << std::endl;
            }
        }
    }
    virtual void sendUserMessage(const std::string& message) override
    {
        std::shared_ptr<boost::asio::streambuf> wrStreambuf = std::make_shared<boost::asio::streambuf>();
        std::ostream os(&(*wrStreambuf));
        std::string output = message + m_chatRoomName + ";" + m_chatClientName + ";";
        os << output + '\n';
        m_tcpClient->sendMessageToServer(wrStreambuf);
    }

    virtual void closeConnection() override
    {
        sendUserMessage(EXIT_THE_CHAT_CMD ";" + m_chatRoomName + ";");
        exit(1);
    }
};
