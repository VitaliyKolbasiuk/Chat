#pragma once

#include <stdint.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <QDebug>
#include <memory>

#include "Types.h"
#include "ed25519/src/ed25519.h"




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
    enum { type = 1 };
    Key         m_publicKey;
    char        m_nickname[64];
    Key         m_deviceKey;
};

// MESSAGE TO CLIENT
struct HandshakeRequest{
    enum { type = 2 };
    std::array<uint8_t, 64> m_random;
};

// MESSAGE TO SERVER
struct HandshakeResponse{
    enum { type = 3 };
    Key                     m_publicKey;
    char                    m_nickname[64];
    Key                     m_deviceKey;
    std::array<uint8_t, 64> m_random;
    Sign                    m_sign;

    void sign(const PrivateKey& privateKey){
        ed25519_sign(&m_sign[0], &m_publicKey[0], sizeof(m_random) + sizeof(m_nickname) + sizeof(m_deviceKey) + sizeof(m_publicKey), &m_publicKey[0], &privateKey[0]);
    }

    bool verify() const {
        return ed25519_verify(&m_sign[0], &m_publicKey[0], sizeof(m_random) + sizeof(m_nickname) + sizeof(m_deviceKey) + sizeof(m_publicKey), &m_publicKey[0]);
    }
};

struct ChatRoomListPacket{
    enum { type = 4};

    void operator delete(void *ptr)
    {
        delete[] reinterpret_cast<uint8_t*>(ptr);
    }

    uint16_t length()
    {
        return reinterpret_cast<PacketHeaderBase*>(this)->length() + sizeof(PacketHeaderBase);
    }

    uint16_t packetType()
    {
        return reinterpret_cast<PacketHeaderBase*>(this)->type();
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
        std::string chatRoomName(length - 1, ' ');
        if (bufferEnd - ptr >= length)
        {
            std::memcpy(&chatRoomName[0], ptr, length);
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
        bufferSize += chatRoom.m_chatRoomName.size() + 2 + 4 + 1;  // length of string + zero-end
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

