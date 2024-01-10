#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <memory>
#include "../Client/ClientInterfaces.h"
#include "QDebug"
#include "Protocol.h"

using namespace boost::asio;
using ip::tcp;

class TcpClient
{
    io_context&  m_ioContext;
    tcp::socket m_socket;
    std::shared_ptr<IChatClient> m_client;

public:
    TcpClient( io_context&  ioContext, std::shared_ptr<IChatClient> client ) :
        m_ioContext(ioContext),
        m_socket(m_ioContext),
        m_client(client)
    {}

    ~TcpClient()
    {
        qCritical() << "!!!! ~Client(): ";
    }

    void connect(const std::string& addr, const int& port)
    {
        qDebug() << "Connect: " << addr << ' ' << port;
        auto endpoint = tcp::endpoint(ip::address::from_string( addr.c_str()), port);

        m_socket.async_connect(endpoint, [this] (const boost::system::error_code& error)
        {
            if ( error )
            {
                qCritical() <<"Connection error: " << error.message();
            }
            else
            {
                qDebug() << "Connection established";
                m_client->onSocketConnected();
            }
        });
    }

    template<typename T>
    void sendPacket(PacketHeader<T>& packet)
    {
        qDebug() << PacketHeader<T>().length() + sizeof(PacketHeaderBase) << sizeof(PacketHeader<CreateChatRoomPacket>);
        qDebug() << "Send Packet buffer length: " << packet.length() << " Type: " << gTypeMap.m_typeMap[packet.type()];
        async_write(m_socket, boost::asio::buffer(&packet, sizeof(PacketHeader<T>)),
                    [this] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                        qDebug() << "Async_write bytes transferred: " << bytes_transferred;
                        if ( ec )
                        {
                            qCritical() << "!!!! Session::sendMessage error (1): " << ec.message();
                            exit(-1);
                        }
                    });
    }

    template<typename T>
    void sendBufferedPacket(const T* packet)
    {
        qDebug() << "Send Packet buffer length: " << packet->length() << " Type: " << gTypeMap.m_typeMap[packet->packetType()];
        async_write(m_socket, boost::asio::buffer(packet, packet->length()),
                    [this, packet] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                        delete packet;
                        if ( ec )
                        {
                            qCritical()<< "!!!! Session::sendMessage error (2): " << ec.message();
                            exit(-1);
                        }
                    });
    }


    void readPacket()
    {
        auto header = std::make_shared<PacketHeaderBase>();
        auto* headerPtr = &(*header);
        async_read(m_socket, buffer(headerPtr, sizeof(*header)), transfer_exactly(sizeof(PacketHeaderBase)),
                   [this, header = std::move(header)] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                       qDebug() << "Async_read bytes transferred: " << bytes_transferred;
                       if ( ec )
                       {
                           qCritical() <<  "!!!! Session::readMessage error (0): " << ec.message();
                           return;
                       }
                       qDebug() << "Async_read header length: " << header->length() << " Header type: " << gTypeMap.m_typeMap[header->type()];
                       if (header->length() == 0)
                       {
                           qCritical() <<  "!!!! Length = 0";
                           return;
                       }
                       auto readBuffer = std::make_shared<std::vector<uint8_t >>(header->length(), 0);

                       async_read( m_socket, buffer(*readBuffer), transfer_exactly(header->length()),
                                   [this, readBuffer, header = *header.get()]
                                           ( const boost::system::error_code& ec, std::size_t bytes_transferred  )
                                   {
                                       if ( ec )
                                       {
                                           qCritical() << "!!!! Session::readMessage error (1): " << ec.message();
                                           return;
                                       }
                                       if (bytes_transferred != header.length())
                                       {
                                           qCritical() << "!!! Bytes transferred doesn't equal length";
                                       }
                                       m_client->onPacketReceived(header.type(), &(*readBuffer)[0], header.length());
                                       readPacket();
                                   });
                   });

    }
};
