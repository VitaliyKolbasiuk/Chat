#include <iostream>
#include <map>
#include <boost/asio.hpp>

//#include "ChatInterfaces.h"
//#include "Utils.h"
//#include "Protocol.h"
#include "ServerSession.h"

using namespace boost::asio;
using ip::tcp;

class TcpServer : public IServer
{
    IChat&          m_chat;

    io_context&     m_ioContext;
    tcp::acceptor   m_acceptor;
    tcp::socket     m_socket;

    std::vector<std::shared_ptr<ServerSession>> m_sessions;

public:
    TcpServer( io_context& ioContext, IChat& chat, int port ) :
        m_chat(chat),
        m_ioContext(ioContext),
        m_acceptor( m_ioContext, tcp::endpoint(tcp::v4(), port) ),
        m_socket(m_ioContext)
    {
    }

    virtual void execute() override
    {
        post( m_ioContext, [this] { accept(); } );
        m_ioContext.run();
    }

    void accept()
    {
        m_acceptor.async_accept( [this] (boost::system::error_code ec, ip::tcp::socket socket ) {
            if (!ec)
            {
                std::make_shared<ServerSession>( m_ioContext, m_chat, std::move(socket) )->readPacket();
            }

            accept();
        });
    }
};

IServer* createServer(io_context& ioContext, IChat& chat, int port)
{
    return new TcpServer(ioContext, chat, port);
}
