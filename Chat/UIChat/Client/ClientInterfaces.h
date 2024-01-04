#pragma once
#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

class IChatClient
{
public:
    virtual ~IChatClient() = default;

public:
    virtual void onSocketConnected() = 0;
    virtual void onPacketReceived ( uint16_t packetType, uint8_t* packet, uint16_t packetSize) = 0;
    virtual void closeConnection() = 0;
};

