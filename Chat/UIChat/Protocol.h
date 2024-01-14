#pragma once

#include <stdint.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <QDebug>
#include <memory>
#include <cmath>

#include "Types.h"
#include "ed25519/src/ed25519.h"
#include "Utils.h"


struct Seed{
    std::array<uint8_t, 64> m_data;
    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_data );
    }
};

struct PacketHeaderBase{
protected:
    uint32_t m_type;
    uint32_t m_length;

public:
    uint32_t type() const
    {
        return m_type;
    }

    void setType(uint32_t type)
    {
        m_type = type;
    }

    uint32_t length() const
    {
        return m_length;
    }

    void setLength(uint32_t length)
    {
        m_length = length;
    }
};

template<typename T>
struct PacketHeader : public PacketHeaderBase{
    PacketHeader() {
        m_type = T::type;
        m_length = sizeof(T);
    }
    T   m_packet;

};

static_assert(sizeof(PacketHeaderBase) + sizeof(uint64_t) == sizeof(PacketHeader<uint64_t>));

// MESSAGE TO SERVER
struct ConnectRequest{
    enum { type = 100 };
    Key         m_publicKey;
    char        m_nickname[64];
    Key         m_deviceKey;
};


// MESSAGE TO CLIENT
struct HandshakeRequest{
    enum { type = 1 };
    std::array<uint8_t, 64> m_random;
};

// MESSAGE TO SERVER
struct HandshakeResponse{
    enum { type = 101 };
    Key                     m_publicKey;
    char                    m_nickname[64];
    Key                     m_deviceKey;
    std::array<uint8_t, 64> m_random;
    Sign                    m_sign;

    void sign(const PrivateKey& privateKey)
    {
        ed25519_sign(&m_sign[0], &m_publicKey[0], sizeof(m_random) + sizeof(m_nickname) + sizeof(m_deviceKey) + sizeof(m_publicKey), &m_publicKey[0], &privateKey[0]);
    }

    bool verify() const
    {
        return ed25519_verify(&m_sign[0], &m_publicKey[0], sizeof(m_random) + sizeof(m_nickname) + sizeof(m_deviceKey) + sizeof(m_publicKey), &m_publicKey[0]);
    }
};

// MESSAGE TO CLIENT
struct ChatRoomListPacket{
    enum { type = 2};

    void operator delete(void *ptr)
    {
        delete[] reinterpret_cast<uint8_t*>(ptr);
    }

    uint16_t length() const
    {
        return reinterpret_cast<const PacketHeaderBase*>(this)->length() + sizeof(PacketHeaderBase);
    }

    uint16_t packetType() const
    {
        return reinterpret_cast<const PacketHeaderBase*>(this)->type();
    }
};

inline ChatRoomInfoList parseChatRoomList(const uint8_t* buffer, size_t bufferSize)
{
    ChatRoomInfoList chatRoomList;
    const uint8_t* bufferEnd = buffer + bufferSize;
    const uint8_t* ptr = buffer;
    while (ptr < bufferEnd)
    {
        uint32_t id;
        if (bufferEnd - ptr >= sizeof(id))
        {
            std::memcpy(&id, ptr, sizeof(id));
            ptr += sizeof(id);
        }
        else
        {
            qWarning() << "Invalid chatRoomListPacket (id)";
            return {};
        }

        uint16_t length;
        if (bufferEnd - ptr >= sizeof(length))
        {
            std::memcpy(&length, ptr, sizeof(length));
            ptr += sizeof(length);
        }
        else
        {
            qWarning() << "Invalid chatRoomListPacket (length)";
            return {};
        }
        if (length == 0)
        {
            qWarning() << "Invalid chatRoomListPacket(length = 0)";
            return {};
        }
        std::string chatRoomName;
        if (bufferEnd - ptr >= length)
        {
            chatRoomName = std::string(reinterpret_cast<const char*>(ptr), length - 1);
            ptr += length;
        }
        else
        {
            qWarning() << "Invalid chatRoomListPacket (name)";
            return {};
        }
        chatRoomList.emplace_back(id, chatRoomName);
    }
    return chatRoomList;
}

inline ChatRoomListPacket* createChatRoomList(const ChatRoomInfoList& chatRoomList)
{
    size_t bufferSize = sizeof(PacketHeaderBase);
    for(const auto& chatRoom : chatRoomList)
    {
        bufferSize += chatRoom.m_chatRoomName.size() + sizeof(ChatRoomId) +sizeof(uint16_t) + 1;  // length of string + zero-end
    }

    auto* buffer = new uint8_t[bufferSize];

    auto* header = reinterpret_cast<PacketHeaderBase*>(buffer);
    header->setType(ChatRoomListPacket::type);
    header->setLength((uint32_t)bufferSize);
    if (bufferSize > 0)
    {
        uint8_t* ptr = buffer + sizeof(PacketHeaderBase);
        for(const auto& chatRoom : chatRoomList)
        {
            std::memcpy(ptr, &chatRoom.m_chatRoomId, sizeof(chatRoom.m_chatRoomId));
            ptr += sizeof(chatRoom.m_chatRoomId);

            uint16_t length = chatRoom.m_chatRoomName.size() + 1;
            std::memcpy(ptr, &length, sizeof(length));
            ptr += sizeof(length);

            std::memcpy(ptr, chatRoom.m_chatRoomName.c_str(), length);
            ptr += length;
        }
    }
    return reinterpret_cast<ChatRoomListPacket*>(buffer);
}

// MESSAGE TO CLIENT
struct ChatRoomUpdatePacket{
    enum { type = 3 };
    ChatRoomId m_chatRoomId;
    char m_chatRoomName[64];
    bool m_addOrDelete;      // ADD - true, DELETE - false
};

// MESSAGE TO SERVER
struct RequestMessagesPacket{
    enum { type = 103 };
    ChatRoomId  m_chatRoomId;
    int         m_messageNumber;
    MessageId   m_messageId = std::numeric_limits<typeof(MessageId::m_id)>::max();
};



struct CreateChatRoomPacket{
    enum { type = 104};
    Key  m_publicKey;
    char m_chatRoomName[64];
    uint32_t m_isPrivate;
    Sign m_sign;

    void sign(const PrivateKey& privateKey)
    {
        ed25519_sign(&m_sign[0], &m_publicKey[0], sizeof(m_publicKey) + sizeof(m_chatRoomName) + sizeof(m_isPrivate), &m_publicKey[0], &privateKey[0]);
    }

    bool verify() const
    {
        return ed25519_verify(&m_sign[0], &m_publicKey[0], sizeof(m_publicKey) + sizeof(m_chatRoomName) + sizeof(m_isPrivate), &m_publicKey[0]);
    }
};


struct SendTextMessagePacket{
    enum {type = 105};

    void operator delete(void *ptr)
    {
        delete[] reinterpret_cast<uint8_t*>(ptr);
    }

    uint16_t length() const
    {
        return reinterpret_cast<const PacketHeaderBase*>(this)->length() + sizeof(PacketHeaderBase);
    }

    uint16_t packetType() const
    {
        return reinterpret_cast<const PacketHeaderBase*>(this)->type();
    }
};

inline SendTextMessagePacket* createSendTextMessagePacket(const std::string& message, ChatRoomId chatRoomId, const Key& publicKey)
{
    size_t bufferSize = sizeof(PacketHeaderBase) + sizeof(uint64_t) + sizeof(chatRoomId) + sizeof(Key) + message.size() + 1;

    auto* buffer = new uint8_t[bufferSize];

    auto* header = reinterpret_cast<PacketHeaderBase*>(buffer);
    header->setType(SendTextMessagePacket::type);
    header->setLength((uint32_t)bufferSize);
    
    *reinterpret_cast<uint64_t*>(buffer + sizeof(*header)) = currentUtc();
    *reinterpret_cast<ChatRoomId*>(buffer + sizeof(*header) + sizeof(uint64_t)) = chatRoomId;
    *reinterpret_cast<Key*>(buffer + sizeof(*header) + sizeof(uint64_t) + sizeof(ChatRoomId)) = publicKey;
    std::memcpy(buffer + sizeof(*header) + sizeof(uint64_t) + sizeof(ChatRoomId) + sizeof(Key), message.c_str(), message.size() + 1);

    return reinterpret_cast<SendTextMessagePacket*>(buffer);
}

inline std::string parseSendTextMessagePacket(const uint8_t* buffer, size_t bufferSize, uint64_t& time, ChatRoomId& chatRoomId, Key& publicKey)
{
    time = *reinterpret_cast<const uint64_t*>(buffer);
    chatRoomId = *reinterpret_cast<const ChatRoomId*>(buffer + sizeof(uint64_t));
    publicKey = *reinterpret_cast<const Key*>(buffer + sizeof(uint64_t) + sizeof(ChatRoomId));
    std::string message(reinterpret_cast<const char*>(buffer) + sizeof(uint64_t) + sizeof(ChatRoomId) + sizeof(Key), bufferSize - sizeof(PacketHeaderBase) - sizeof(uint64_t) - sizeof(ChatRoomId) - sizeof(Key) - 1);

    return message;
}

struct TextMessagePacket{
    enum {type = 4};

    void operator delete(void *ptr)
    {
        delete[] reinterpret_cast<uint8_t*>(ptr);
    }

    uint16_t length() const
    {
        return reinterpret_cast<const PacketHeaderBase*>(this)->length() + sizeof(PacketHeaderBase);
    }

    uint16_t packetType() const
    {
        return reinterpret_cast<const PacketHeaderBase*>(this)->type();
    }
};

inline TextMessagePacket* createTextMessagePacket(const std::string& message, MessageId& messageId, ChatRoomId& chatRoomId, const UserId& userId, const std::string& username, uint64_t time)
{
    size_t bufferSize = sizeof(PacketHeaderBase) + sizeof(messageId) + sizeof(uint64_t) + sizeof(chatRoomId) + sizeof(UserId) + 1 + username.size() + message.size() + 1; // first +1 is a size of username

    auto* buffer = new uint8_t[bufferSize];

    auto* header = reinterpret_cast<PacketHeaderBase*>(buffer);
    header->setType(TextMessagePacket::type);
    header->setLength((uint32_t)bufferSize);

    auto* ptr = buffer + sizeof(*header);

    *reinterpret_cast<MessageId*>(ptr) = messageId;
    ptr += sizeof(MessageId);

    *reinterpret_cast<uint64_t*>(ptr) = currentUtc();
    ptr += sizeof(uint64_t);

    *reinterpret_cast<ChatRoomId*>(ptr) = chatRoomId;
    ptr += sizeof(chatRoomId);

    *reinterpret_cast<UserId*>(ptr) = userId;
    ptr += sizeof(UserId);

    size_t usernameSize = std::min(username.size(), (size_t)255);
    *ptr = (uint8_t)usernameSize;
    ptr += 1;

    std::memcpy(ptr, username.c_str(), usernameSize);
    ptr += usernameSize;

    std::memcpy(ptr, message.c_str(), message.size() + 1);

    return reinterpret_cast<TextMessagePacket*>(buffer);
}

inline std::string parseTextMessagePacket(const uint8_t* buffer, size_t bufferSize, uint64_t& time, ChatRoomId& chatRoomId, MessageId& messageId, UserId& userId, std::string& username)
{
    auto* ptr = buffer;

    messageId = *reinterpret_cast<const MessageId*>(ptr);
    ptr += sizeof(MessageId);

    time = *reinterpret_cast<const uint64_t*>(ptr);
    ptr += sizeof(uint64_t);

    chatRoomId = *reinterpret_cast<const ChatRoomId*>(ptr);
    ptr += sizeof(ChatRoomId);

    userId = *reinterpret_cast<const UserId*>(ptr);
    ptr += sizeof(UserId);

    size_t usernameSize = *ptr;
    ptr += 1;

    username = std::string(reinterpret_cast<const char*>(ptr), usernameSize);
    ptr += usernameSize;

    std::string message(reinterpret_cast<const char*>(ptr), buffer + bufferSize - ptr - 1);

    return message;
}

struct TypeMap{
    std::map<int, std::string> m_typeMap;
    TypeMap(){
        m_typeMap[ConnectRequest::type] = "<ConnectRequest>";
        m_typeMap[HandshakeRequest::type] = "<HandshakeRequest>";
        m_typeMap[HandshakeResponse::type] = "<HandshakeResponse>";
        m_typeMap[ChatRoomListPacket::type] = "<ChatRoomListPacket>";
        m_typeMap[RequestMessagesPacket::type] = "<RequestMessagesPacket>";
        m_typeMap[CreateChatRoomPacket::type] = "<CreateChatRoomPacket>";
        m_typeMap[ChatRoomUpdatePacket::type] = "<ChatRoomUpdatePacket>";
        m_typeMap[SendTextMessagePacket::type] = "<SendTextMessagePacket>";
        m_typeMap[TextMessagePacket::type] = "<TextMessagePacket>";
    }
};

inline TypeMap gTypeMap;

