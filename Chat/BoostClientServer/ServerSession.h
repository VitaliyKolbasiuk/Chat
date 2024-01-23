//
// Created by vitaliykolbasiuk on 23.11.23.
//
#pragma once

#include <iostream>
#include <map>
#include <boost/asio.hpp>

#include "ChatInterfaces.h"
#include "Utils.h"
#include "Protocol.h"

class ServerSession : public std::enable_shared_from_this<ServerSession>
{
    io_context&             m_ioContext;
    IChat&                  m_chat;
    tcp::socket             m_socket;
    boost::asio::streambuf  m_streambuf;

public:
    ServerSession( io_context& ioContext, IChat& chat, tcp::socket&& socket)
            : m_ioContext(ioContext),
              m_chat(chat),
              m_socket(std::move(socket))
    {
        //async_read( m_socket );
    }

    ~ServerSession() { qCritical() << "!!!! ~ClientSession()"; }

    template<typename T>
    void sendPacket(PacketHeader<T>& packet)
    {
        qDebug() << ">>> Send packet packetType&: " << gTypeMap.m_typeMap[packet.packetType()] << " size of packet: " << sizeof(PacketHeader<T>);
        async_write(m_socket, boost::asio::buffer(&packet, sizeof(PacketHeader<T>)),
                    [this] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                        if ( ec )
                        {
                            qCritical()<< "!!!! Session::sendMessage error (1): " << ec.message();
                            exit(-1);
                        }
                    });
    }

    template<typename T>
    void sendPacket(PacketHeader<T>* packet)
    {
        qDebug() << ">>> Send packet packetType*: " << gTypeMap.m_typeMap[packet->packetType()] << " size of packet: " << sizeof(PacketHeader<T>);
        async_write(m_socket, boost::asio::buffer(&packet, sizeof(PacketHeader<T>)),
                    [this, packet] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                        delete packet;
                        if ( ec )
                        {
                            qCritical()<< "!!!! Session::sendMessage error (1): " << ec.message();
                            exit(-1);
                        }
                    });
    }

    template<typename T>
    void sendBufferedPacket(T* packet)
    {
        qDebug() << ">>> Send packet packetType: " << gTypeMap.m_typeMap[packet->packetType()] << " size of packet: " << packet->packetLength();
        async_write(m_socket, boost::asio::buffer(packet, packet->packetLength()),
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
                       qDebug() << "Async_read packet packetLength : " << header->packetLength() << " Packet packetType: " << gTypeMap.m_typeMap[header->packetType()];
                       if (header->packetLength() == 0)
                       {
                           m_chat.onPacketReceived(header->packetType(), nullptr,
                                                   header->packetLength(), weak_from_this());
                           return;
                       }
                       auto readBuffer = std::make_shared<std::vector<uint8_t >>(header->packetLength(), 0);

                       async_read( m_socket, buffer(*readBuffer), transfer_exactly(header->packetLength()),
                                   [this, readBuffer, header = *header.get()]
                                           ( const boost::system::error_code& ec, std::size_t bytes_transferred  )
                                   {
                                       if ( ec )
                                       {
                                           qCritical()<< "!!!! Session::readMessage error (1): " << ec.message();
                                           return;
                                       }
                                       if (bytes_transferred != header.packetLength())
                                       {
                                           qCritical() << "!!! Bytes transferred doesn't equal packetLength";
                                       }
                                       m_chat.onPacketReceived(header.packetType(), &(*readBuffer)[0],
                                                               header.packetLength(), weak_from_this());
                                       readPacket();
                                   });
                   });

    }

    void closeConnection()
    {
        m_socket.close();
    }
};