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

    ~ServerSession() { std::cout << "!!!! ~ClientSession()" << std::endl; }

    template<typename T>
    void sendPacket(RequestHeader<T>& buffer)
    {
        async_write(m_socket, boost::asio::buffer(&buffer, sizeof(T)),
                    [this] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                        if ( ec )
                        {
                            std::cout << "!!!! Session::sendMessage error (1): " << ec.message() << std::endl;
                            exit(-1);
                        }
                    });
    }

    void readPacket()
    {
        auto header = std::make_shared<RequestHeaderBase>();


        async_read(m_socket, mutable_buffer((void*)header.get(), sizeof(*header)), transfer_all(),
                   [this, header = std::move(header)] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                       if ( ec )
                       {
                           qDebug() <<  "!!!! Session::sendMessage error (0): " << ec.message();
                           return;
                       }
                       if (header->m_length == 0)
                       {
                           qDebug() <<  "!!!! Length = 0";
                           return;
                       }
                       auto readBuffer = std::make_shared<std::vector<uint8_t >>(header->m_length, 0);

                       async_read( m_socket, mutable_buffer(&readBuffer.get()[0], header->m_length+ sizeof(uint16_t)), boost::asio::transfer_all(),
                                   [this, readBuffer = std::move(readBuffer), header = *header.get()]
                                           ( const boost::system::error_code& ec, std::size_t bytes_transferred  )
                                   {
                                       if ( ec )
                                       {
                                           std::cout << "!!!! Session::sendMessage error (1): " << ec.message()
                                                     << std::endl;
                                           return;
                                       }
                                       if (bytes_transferred != header.m_length)
                                       {
                                           qDebug() << "!!! Bytes transferred doesn't equal length";
                                       }
                                       m_chat.onPacketReceived(header.m_type, &(*readBuffer)[0], header.m_length, weak_from_this());
                                   });
                   });
    }
};