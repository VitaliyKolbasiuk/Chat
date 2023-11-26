#pragma once

#include <stdint.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <QDebug>

#include "Types.h"




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
struct RequestHeader : public RequestHeaderBase{
    RequestHeader() {
        m_type = T::type;
        m_length = sizeof(T);
    }
    T   m_request;

};

static_assert(sizeof(RequestHeaderBase) + sizeof(uint64_t) == sizeof(RequestHeader<uint64_t>));

// MESSAGE TO SERVER
struct ConnectRequest{
    enum { type = 1 };
    Key         m_publicKey;
    std::string m_nickname;
    Key         m_deviceKey;
};

// MESSAGE TO CLIENT
struct HandShakeRequest{
    enum { type = 2 };
    std::array<uint8_t, 64> m_random;
};

// MESSAGE TO SERVER
struct HandShakeResponse{
    enum { type = 3 };
    Sign m_sign;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_sign );
    }
};



