#pragma once

#include "Types.h"

class IChatClient
{
public:
    virtual ~IChatClient() = default;

public:
    virtual void onSocketConnected() = 0;
    virtual void onPacketReceived ( uint16_t packetType, uint8_t* packet, uint16_t packetSize) = 0;
    virtual void closeConnection() = 0;
};

