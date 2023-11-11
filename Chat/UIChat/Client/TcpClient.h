#pragma once
#include <iostream>
#include <boost/asio.hpp>
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
        std::cout << "!!!! ~Client(): " << std::endl;
    }

    void execute( std::string addr, int port)
    {
        auto endpoint = tcp::endpoint(ip::address::from_string( addr.c_str()), port);

        m_socket.async_connect(endpoint, [this] (const boost::system::error_code& error)
                               {
                                   if ( error )
                                   {
                                       std::cout << "Connection error: " << error.message() << std::endl;
                                   }
                                   else
                                   {
                                       std::shared_ptr<boost::asio::streambuf> wrStreambuf = std::make_shared<boost::asio::streambuf>();
                                       std::ostream os(&(*wrStreambuf));

                                       //sendMessageToServer( wrStreambuf );
                                   }
                               });
    }

    void sendPacket(const AutoBuffer& buffer)
    {
        m_socket.async_send(boost::asio::buffer(buffer.m_buffer, sizeof(uint16_t) + sizeof(uint16_t) + buffer.getDataSize()), [this, buffer] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
            if ( ec )
            {
                std::cout << "!!!! Session::sendMessage error (0): " << ec.message() << std::endl;
                exit(-1);
            }
        });
    }

    void readPacket(const std::string& packet)
    {
        auto length = std::make_unique<uint16_t>();

        async_read(m_socket, mutable_buffer((void*)length.get(), sizeof(*length)), transfer_all(),
                   [this, length = std::move(length)] (const boost::system::error_code& ec, std::size_t bytes_transferred ) {
            if ( ec )
            {
                std::cout << "!!!! Session::sendMessage error (0): " << ec.message() << std::endl;
                return;
            }
            if (*length == 0)
            {
                std::cout << "!!!! Length = 0";
                return;
            }
                       auto readBuffer = std::make_unique<std::vector<uint8_t >>(*length + sizeof(uint16_t), 0);

            async_read( m_socket, mutable_buffer(&readBuffer.get()[0], *length + sizeof(uint16_t)), boost::asio::transfer_all(),
                                 [this, readBuffer = std::move(readBuffer), length = *length] ( const boost::system::error_code& ec, std::size_t bytes_transferred  )
                                 {
                                     if ( ec )
                                     {
                                         std::cout << "!!!! Session::sendMessage error (1): " << ec.message()
                                                   << std::endl;
                                         return;
                                     }
                                     uint16_t packetType = *(reinterpret_cast<uint16_t*>(&(*readBuffer)[0]));
                                     m_client->onPacketReceived(packetType, &(*readBuffer)[0] + sizeof(uint16_t), length);
                                 });
        });
    }



    void readResponse()
    {
        // Receive the response from the server
        std::shared_ptr<boost::asio::streambuf> streambuf = std::make_shared<boost::asio::streambuf>();

        boost::asio::async_read_until( m_socket, *streambuf, '\n',
                                      [streambuf,this]( const boost::system::error_code& error_code, std::size_t bytes_transferred )
                                      {
                                          if ( error_code )
                                          {
                                              std::cout << "Client read error: " << error_code.message() << std::endl;
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
