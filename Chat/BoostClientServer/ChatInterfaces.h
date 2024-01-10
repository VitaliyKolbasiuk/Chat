#pragma once
#include <iostream>
#include <boost/asio.hpp>

#include "Types.h"

using namespace boost::asio;
using ip::tcp;

inline io_context gDatabaseIoContext;
inline boost::asio::executor_work_guard<decltype(gDatabaseIoContext.get_executor())> work{gDatabaseIoContext.get_executor()};
inline io_context gServerIoContext;

class ServerSession;

class IChat
{
protected:
    virtual ~IChat() = default;

public:
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
    virtual ChatRoomInfoList getChatRoomList(const int& userUniqueKey) = 0;
    virtual void onUserConnected(const Key& publicKey, const Key& deviceKey, const std::string& nickname, std::weak_ptr<ServerSession> session) = 0;
    virtual void createChatRoomTable(const std::string& chatRoomName, bool isPrivate, Key ownerPublicKey, std::weak_ptr<ServerSession> session) = 0;
};

IChatDatabase* createDatabase();

IServer* createServer(io_context& ioContext, IChat& chat, int port);
