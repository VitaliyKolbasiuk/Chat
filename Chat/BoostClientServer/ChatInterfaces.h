#pragma once

#include <boost/asio.hpp>

#include "Types.h"

using namespace boost::asio;
using ip::tcp;

inline io_context gDatabaseIoContext;
inline boost::asio::executor_work_guard<decltype(gDatabaseIoContext.get_executor())> work{gDatabaseIoContext.get_executor()};
inline io_context gServerIoContext;

class ServerSession;

class IChatModel
{
protected:
    virtual ~IChatModel() = default;

public:
    virtual void onPacketReceived(uint16_t packetType, const uint8_t* readBuffer, uint16_t length, std::weak_ptr<ServerSession> session) = 0;
    virtual void updateChatRoomList(const std::string& chatRoomName, uint32_t id, bool isAdd, bool isOwner, std::weak_ptr<ServerSession> session) = 0;
    virtual void connectToChatRoomFailed(const std::string& chatRoomName, std::weak_ptr<ServerSession> session) = 0;

    virtual void closeConnection(ServerSession& serverSession) = 0;
};

class IServer //: public std::enable_shared_from_this<IServer>
{
public:
    virtual ~IServer() = default;
    virtual void execute() = 0;
    virtual void removeSession(ServerSession&) = 0;
};

class IChatDatabase
{
public:
    virtual ~IChatDatabase() = default;
    virtual void test() = 0;
    virtual void readChatRoomCatalogue(std::function<void(uint32_t, const std::string&, const std::string&, const Key&, bool)> func) = 0;
    virtual ChatRoomInfoList getChatRoomList(const int& userUniqueKey) = 0;
    virtual void onUserConnected(const Key& publicKey, const Key& deviceKey, const std::string& nickname, std::function<void(const ChatRoomInfoList&)>) = 0;
    virtual void onRequestMessages(ChatRoomId chatRoomId, int messageNumber, MessageId messageId,  std::function<void(const std::vector<ChatRoomRecord>&)>) = 0;
    virtual int createChatRoomTable(const std::string& chatRoomName, bool isPrivate, Key ownerPublicKey, std::weak_ptr<ServerSession> session) = 0;
    virtual uint32_t appendMessageToChatRoom(int chatRoomId, const Key& publicKey, uint64_t dataTime, const std::string& message, int& senderId) = 0;
    virtual bool getUserId(const Key& publicKey, int& userId) = 0;
    virtual void onConnectToChatRoomMessage(const std::string& chatRoomName, Key userKey, std::weak_ptr<ServerSession> session) = 0;
    virtual void leaveChatRoom(ChatRoomId chatRoomId, Key userKey, bool onlyLeave) = 0;
    virtual void deleteMessage(ChatRoomId chatRoomId, MessageId messageId, Key userKey, std::function<void()>) = 0;
};

IChatDatabase* createDatabase(IChatModel& chat);

std::shared_ptr<IServer> createServer(io_context& ioContext, IChatModel& chat, int port);
