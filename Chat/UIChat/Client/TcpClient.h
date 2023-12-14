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

    void connect(std::string addr, int port)
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
    void sendPacket(PacketHeader<T>& buffer)
    {
        async_write(m_socket, boost::asio::buffer(&buffer, sizeof(PacketHeader<T>)),
                            [this] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
            qDebug() << "Async_write bytes transferred: " << bytes_transferred;
            if ( ec )
            {
                qCritical() << "!!!! Session::sendMessage error (1): " << ec.message();
                exit(-1);
            }
        });
    }

    void readPacket()
    {
        auto header = std::make_shared<RequestHeaderBase>();
        auto* headerPtr = &(*header);
        async_read(m_socket, buffer(headerPtr, sizeof(*header)), transfer_exactly(sizeof(RequestHeaderBase)),
                   [this, header = std::move(header)] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
                       qDebug() << "Async_read bytes transferred: " << bytes_transferred;
                       if ( ec )
                       {
                           qCritical() <<  "!!!! Session::readMessage error (0): " << ec.message();
                           return;
                       }
                       qDebug() << "Async_read: " << header->m_length << ' ' << header->m_type;
                       if (header->m_length == 0)
                       {
                           qCritical() <<  "!!!! Length = 0";
                           return;
                       }
                       auto readBuffer = std::make_shared<std::vector<uint8_t >>(header->m_length, 0);

                       async_read( m_socket, buffer(*readBuffer), transfer_exactly(header->m_length),
                                   [this, readBuffer, header = *header.get()]
                                           ( const boost::system::error_code& ec, std::size_t bytes_transferred  )
                                   {
                                       if ( ec )
                                       {
                                           qCritical() << "!!!! Session::readMessage error (1): " << ec.message();
                                           return;
                                       }
                                       if (bytes_transferred != header.m_length)
                                       {
                                           qCritical() << "!!! Bytes transferred doesn't equal length";
                                       }
                                       m_client->onPacketReceived(header.m_type, &(*readBuffer)[0], header.m_length);
                                       readPacket();
                                   });
                   });

    }



    void readResponse()
    {
        // Receive the response from the server
        std::shared_ptr<boost::asio::streambuf> streambuf = std::make_shared<boost::asio::streambuf>();

        boost::asio::async_read_until( m_socket, *streambuf, '\n', [streambuf,this]( const boost::system::error_code& error_code, std::size_t bytes_transferred )
        {
            if ( error_code )
            {
                qCritical()<< "Client read error: " << error_code.message();
            }
            else
            {
                {
                    std::istream response( &(*streambuf) );

                    std::string command;
                    std::getline( response, command, ';' );

                    m_client->handleServerMessage( command, *streambuf );

                }

                streambuf->consume(bytes_transferred);
                readResponse();
            }
        });
    }
};
