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
        return reinterpret_cast<PacketHeaderBase*>(this)->length();
    }

    uint16_t packetType()
    {
        return reinterpret_cast<PacketHeaderBase*>(this)->type();
    }
};

inline std::vector<std::string> parseChatRoomList(const uint8_t* buffer)
{
    // TODO
    return {};
}

inline ChatRoomListPacket* createChatRoomList(const std::vector<std::string>& chatRoomList)
{
    size_t bufferSize = sizeof(PacketHeaderBase);
    for(const auto& chatRoom : chatRoomList)
    {
        bufferSize += chatRoom.size() + 2 + 1;  // length of string + zero-end
    }

    uint8_t* buffer = new uint8_t[bufferSize];

    PacketHeaderBase* header = reinterpret_cast<PacketHeaderBase*>(buffer);
    header->setType(ChatRoomListPacket::type);
    header->setLength((uint32_t)bufferSize);
    if (bufferSize > 0)
    {
        uint8_t* ptr = buffer + sizeof(PacketHeaderBase);
        for(const auto& chatRoom : chatRoomList)
        {
            uint16_t length = chatRoom.size() + 1;
            std::memcpy(ptr, &length, sizeof(length));
            ptr += sizeof(length);
            std::memcpy(ptr, chatRoom.c_str(), length);
        }
    }
    return reinterpret_cast<ChatRoomListPacket*>(buffer);
}

