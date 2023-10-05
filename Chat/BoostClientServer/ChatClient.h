#pragma once

#include <iostream>
#include <QTimer>

#include "ChatInterfaces.h"
#include "TcpClient.h"


class ChatClient : public IChatClient
{
    std::string m_chatClientName;
    TcpClient* m_tcpClient = nullptr;

public:
    ChatClient()
    {
        //        std::cout << "Enter your name" << std::endl;
        //        std::cin >> m_chatClientName;
        //        std::cout << "Hello, " << m_chatClientName << "!";
    }

    void setTcpClient( TcpClient* tcpClient ) { m_tcpClient = tcpClient; }

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
            input >> username;
            std::getline(input, message, ';');
            std::cout << '[' << username << "]: " << message;
        }
    }

};
