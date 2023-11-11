#pragma once

#include <stdint.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

struct AutoBuffer{
    uint8_t* m_buffer = nullptr;

    AutoBuffer(uint16_t packetType, const std::string data) {
        assert(data.size() < INT16_MAX);
        m_buffer = new uint8_t(data.size() + sizeof(uint16_t) + sizeof(uint16_t));
        *(reinterpret_cast<uint16_t*>(m_buffer)) = data.size();
        *(reinterpret_cast<uint16_t*>(m_buffer + sizeof(uint16_t))) = packetType;
        std::memcpy(m_buffer + sizeof(uint16_t) + sizeof(uint16_t), data.c_str(), data.size());
    }

    template<typename Type>
    static AutoBuffer createBuffer(const Type& data)
    {
        std::stringstream ss;
        cereal::BinaryOutputArchive archive( ss );
        archive(data);

        return AutoBuffer(Type::type, ss.str());
    }

    AutoBuffer(const AutoBuffer& autoBuffer){
        m_buffer = autoBuffer.m_buffer;
        ((AutoBuffer&)autoBuffer).m_buffer = nullptr;
    }

    ~AutoBuffer(){
        delete m_buffer;
    }

    uint16_t getPacketType() const
    {
        return *(reinterpret_cast<uint16_t*>(m_buffer + sizeof(uint16_t)));
    }

    uint16_t getDataSize() const
    {
        return *(reinterpret_cast<uint16_t*>(m_buffer));
    }

    uint8_t* getData()
    {
        return m_buffer + sizeof(uint16_t) + sizeof(uint16_t);
    }
};

struct Seed{
    std::array<uint8_t, 64> m_data;
    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_data );
    }
};

// MESSAGE TO SERVER
struct ConnectRequest{
    enum { type = 1 };
    std::array<uint8_t, 32> m_publicKey;
    std::string m_nickname;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_publicKey, m_nickname );
    }
};

// MESSAGE TO CLIENT
struct HandShakeRequest{
    enum { type = 2 };
    std::array<uint8_t, 64> m_random;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_random );
    }
};

// MESSAGE TO SERVER
struct HandShakeResponse{
    enum { type = 3 };
    std::array<uint8_t, 64> m_sign;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_sign );
    }
};



