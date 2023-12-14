#pragma once

#include <stdint.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <QDebug>

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

struct RequestHeaderBase{
    uint32_t m_type;
    uint32_t m_length;
};

template<typename T>
struct PacketHeader : public RequestHeaderBase{
    PacketHeader() {
        m_type = T::type;
        m_length = sizeof(T);
    }
    T   m_packet;

};

static_assert(sizeof(RequestHeaderBase) + sizeof(uint64_t) == sizeof(PacketHeader<uint64_t>));

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



