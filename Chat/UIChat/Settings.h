#pragma once

#include <cereal/types/string.hpp>
#include <array>

struct KeyPair{
    std::array<uint8_t, 64> m_privateKey;
    std::array<uint8_t, 32> m_publicKey;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_privateKey, m_publicKey );
    }
};

class Settings{
    //std::string m_historyFileName;

    std::string settingsFileName(){
        return "settings3.bin";
    }

public:
    int m_version = 0;
    KeyPair m_keyPair;
    std::array<uint8_t, 32> m_deviceKey;
    std::string m_username;

    void loadSettings();

    void saveSettings();

    void generateKeys();

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( m_version );
        ar( m_deviceKey );
        ar( m_username );
    }
};
