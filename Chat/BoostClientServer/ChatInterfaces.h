#pragma once
#include <iostream>
#include <boost/asio.hpp>

// messages from server to client
#define MESSAGE_FROM_CMD    "MESSAGE_FROM" // cmd;nick_name;message

// messages from client to seraver
#define JOIN_TO_CHAT_CMD    "JOIN_TO_CHAT"    // cmd;nick_name
#define MESSAGE_TO_ALL_CMD  "MESSAGE_TO_ALL"  // cmd;message

using namespace boost::asio;
using ip::tcp;

class IClientSessionUserData
{
protected:
    virtual ~IClientSessionUserData() = default;
};

class IClientSession
{
public:
    virtual ~IClientSession() = default;

    virtual void sendMessage( std::string message ) = 0;
    virtual void sendMessage( std::shared_ptr<boost::asio::streambuf> wrStreambuf ) = 0;

    virtual void  setUserInfoPtr( std::weak_ptr<IClientSessionUserData> userInfoPtr ) = 0;
    virtual std::weak_ptr<IClientSessionUserData> getUserInfoPtr() = 0;
};

class IChat
{
protected:
    virtual ~IChat() = default;

public:
    virtual void handleMessage( IClientSession&, boost::asio::streambuf& message ) = 0;
};

class IChatClient
{
protected:
    virtual ~IChatClient() = default;

public:
    virtual void handleServerMessage( const std::string& command, boost::asio::streambuf& message ) = 0;
};

