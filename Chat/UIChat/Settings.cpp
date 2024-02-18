#include "Settings.h"
#include "Utils.h"

#define FIXEDINT_H_INCLUDED

#include <cereal/archives/binary.hpp>
#include <fstream>
#include <QUuid>

#include "ed25519/src/ed25519.h"

void Settings::generateKeys()
{
    unsigned char scalar[32];
    ed25519_create_seed(scalar);
    ed25519_create_keypair(&m_keyPair.m_publicKey[0], &m_keyPair.m_privateKey[0], scalar);

    qDebug() << "!!!!!Keypair generated";

    generateRandomKey(m_deviceKey);

    QUuid uuid = QUuid::createUuid();
    auto uuidString = uuid.toString().toStdString();
    Sign sign;
    ed25519_sign(&sign[0], (uint8_t*)uuidString.c_str(), uuidString.size(), &m_keyPair.m_publicKey[0], &m_keyPair.m_privateKey[0]);

    // Output the UUID
    std::memcpy(&m_deviceKey, &sign[0], 32);
}

bool Settings::loadSettings()
{
    try{
        std::ifstream ios(settingsFileName() + "_keys", std::ios::binary);
        cereal::BinaryInputArchive archive( ios );
        archive(*this);
        return true;
    }
    catch(std::runtime_error& ex){
        qDebug() << "Cannot load settings: " << ex.what();
        return false;
    }

    // TEST CASE
//    unsigned char signature[64];
//    ed25519_sign(signature, (unsigned char*)"message", 7, &m_keyPair.m_publicKey[0], &m_keyPair.m_privateKey[0]);

    /* verify the signature */
//    if (ed25519_verify(signature, (unsigned char*)"message", 7, &m_keyPair.m_publicKey[0])) {
//        qDebug() << "valid signature";
//    } else {
//        qDebug() << "invalid signature";
//    }

    try{
        std::ifstream ios(settingsFileName(), std::ios::binary);
        cereal::BinaryInputArchive archive( ios );
        archive(*this);
    }
    catch(...){
        if (m_version == 0)
        {
            m_version = 1;
        }
        else
        {
            // TO DO
            qDebug() << "Settings corrupted";
            exit(-1);
        }
        saveSettings();
    }

}

void Settings::saveSettings()
{
    try{
        std::ofstream os(settingsFileName() + "_keys", std::ios::binary);
        cereal::BinaryOutputArchive archive( os );
        archive(*this);
    }
    catch(std::exception& ex){
        qDebug() << "Save Setting exception: " << ex.what();
    }
}
