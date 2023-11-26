#pragma once
#include <iostream>
#include <boost/asio.hpp>

// messages from server to client
#define CHAT_ROOM_LIST_CMD        "CHAT_ROOM_LIST"        // cmd;{chat_room_name;chat_room_id;}*
#define MESSAGE_FROM_CMD          "MESSAGE_FROM"          // cmd;message;chat_room;username
#define UPDATE_THE_USER_TABLE_CMD "UPDATE_THE_USER_TABLE" // cmd;message;

// messages from client to server
#define CONNECT_CMD          "CONNECT"           // cmd;user_unique_key;device_unique_key
#define CREATE_CHAT_ROOM_CMD "CREATE_CHAT_ROOM"  // cmd;chatroom_name;is_private;owner_public_key
#define JOIN_TO_CHAT_CMD     "JOIN_TO_CHAT"      // cmd;chat_room;username
#define MESSAGE_TO_ALL_CMD   "MESSAGE_TO_ALL"    // cmd;message;chat_room;username
#define EXIT_THE_CHAT_CMD    "EXIT_THE_CHAT"     // cmd;chatroom;username

using namespace boost::asio;
using ip::tcp;

class ServerSession;

class IChat
{
protected:
    virtual ~IChat() = default;

public:
    //virtual void handleMessage(ServerSession&, boost::asio::streambuf& message ) = 0;
    virtual void onPacketReceived(uint16_t packetType, const uint8_t* readBuffer, uint16_t length, std::weak_ptr<ServerSession> session) = 0;
    //virtual void closeConnection(Session& client) = 0;
};

class IServer
{
public:
    virtual ~IServer() = default;
    virtual void execute() = 0;
};

class IChatDatabase
{
public:
    virtual ~IChatDatabase() = default;
    virtual void test() = 0;
    virtual std::vector<std::string> getChatRoomList(std::string userUniqueKey) = 0;
};

IChatDatabase* createDatabase();

IServer* createServer(io_context& ioContext, IChat& chat, int port);
