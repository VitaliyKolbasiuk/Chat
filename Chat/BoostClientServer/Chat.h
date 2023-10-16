#include <iostream>
#include <list>
#include <map>
#include <optional>

#include "ChatInterfaces.h"

class ChatRoom;

struct User
{
    ChatRoom* m_room = nullptr;
    ISession* m_session;
    std::string m_username;

    User(ChatRoom* room = nullptr, ISession* session = nullptr) : m_room(room), m_session(session) {}

    User (const User&) = default;
    User& operator=(const User&) = default;
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

    std::map<std::string, ChatRoom> m_chatRooms;

public:
    Chat() {}

    virtual void handleMessage(ISession& client, boost::asio::streambuf& message) override
    {
        std::istringstream input;
        input.str( std::string( (const char*)message.data().data(), message.size() ) );

        std::string command;
        std::getline( input, command, ';');
        if ( command == JOIN_TO_CHAT_CMD )
        {
            std::string chatRoomName;
            std::getline( input, chatRoomName, ';');

            std::string username;
            std::getline( input, username, ';');

            if (auto chatRoom = m_chatRooms.find(chatRoomName); chatRoom == m_chatRooms.end()){
                m_chatRooms[chatRoomName] = ChatRoom();
            }

            User newClient(&m_chatRooms[chatRoomName], &client);
            newClient.m_username = username;
            m_chatRooms[chatRoomName].addClient(newClient);

            std::shared_ptr<boost::asio::streambuf> wrStreambuf = std::make_shared<boost::asio::streambuf>();
            std::ostream os(&(*wrStreambuf));

            for (const auto &it : m_chatRooms[chatRoomName].clients())
            {
                os << MESSAGE_FROM_CMD";" << username << " has joined the chat;Server;\n";
                it.second.m_session->sendMessage(wrStreambuf);
            }
        }
        else if ( command == MESSAGE_TO_ALL_CMD )
        {
            std::string message, chatRoomName, username;
            std::getline(input, message, ';');
            std::getline(input, chatRoomName, ';');
            std::getline(input, username, ';');
            std::shared_ptr<boost::asio::streambuf> wrStreambuf = std::make_shared<boost::asio::streambuf>();
            std::ostream os(&(*wrStreambuf));
            for (const auto &it : m_chatRooms[chatRoomName].clients())
            {
                os << MESSAGE_FROM_CMD ";" << message << ";" << username << ";\n";
                it.second.m_session->sendMessage(wrStreambuf);
            }
        }
        else if ( command == EXIT_THE_CHAT_CMD )
        {
            std::string roomName, username;
            std::getline(input, roomName, ';');
            std::getline(input, username, ';');
            User goneUser(&m_chatRooms[roomName], &client);
            goneUser.m_username = username;
            m_chatRooms[roomName].removeClient(goneUser);
//            for (const auto &it : m_chatRooms[roomName].m_clients)
//            {
//                std::cout << it.first << std::endl;
//            }
        }
    }

//    virtual void closeConnection(ISession& client) override
//    {

//    }
};
