#include <iostream>
#include <list>
#include <optional>

#include "BoostClientServer/ChatInterfaces.h"

//------------------------------------------------------------------
// Client->Server: StartGame, MatchId, width, height ( GameId random sequesnce)
// Server->Client: WaitingGame
//------------------------------------------------------------------
// Server->Client: GameStarded, direction [left|right]
// Server->Client: Ball, x,y,dx,dy
// Client->Server: ToolPosition, x, y
// Server->Client: Score, number1, number2
//------------------------------------------------------------------

class ChatRoom;

struct Client : public IClientSessionUserData
{
    ChatRoom& m_room;
    IClientSession* m_session;
    std::string username;

    Client(ChatRoom& room, IClientSession* session)
        : m_room(room)
        , m_session(session)
    {
    }
};

class ChatRoom
{
    io_context&             m_serverIoContext;
    
public:
    std::list<Client> Clients;
    std::mutex mutex;

private:
    boost::asio::high_resolution_timer m_timer;
    
public:
    ChatRoom(io_context& serverIoContext)
        : m_serverIoContext(serverIoContext)
        , m_timer(m_serverIoContext)
    {
    }

public:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp;

    void start()
    {
        m_lastTimestamp = std::chrono::high_resolution_clock::now();

        // delay on start
        m_timer.expires_after( std::chrono::milliseconds( 3000 ));
        m_timer.async_wait([this](const boost::system::error_code& ec )
        {
            if ( ec )
            {
                exit(1);
            }
            tick();
        });
    }

    void addClient(Client client) { Clients.push_back(client); }

    int startCounter = 0;
    const int skipNumber = 1;

    void tick()
    {
        m_timer.expires_after( std::chrono::milliseconds( 30 ));
        m_timer.async_wait([this](const boost::system::error_code& ec )
        {
            if ( ++startCounter < skipNumber )
            {
                tick();
                return;
            }
            else if ( startCounter == skipNumber )
            {
                m_lastTimestamp = std::chrono::high_resolution_clock::now();
                tick();
                return;
            }
            
            if ( ec )
            {
                exit(1);
            }
            
            auto durationMs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_lastTimestamp);
            m_lastTimestamp = std::chrono::high_resolution_clock::now();

            tick();
        });
    }
    
    void sendUpdateScene()
    {
        std::istringstream input;
        input.str(std::string((const char*) message.data().data(), message.size()));

        std::string
    }
};

class Chat: public IChat
{
    io_context& m_serverIoContext;

    std::list<ChatRoom> m_chatRooms;

public:
    Chat(io_context& serverIoContext)
        : m_serverIoContext(serverIoContext)
    {
    }

    virtual void handleMessage(IClientSession& client, boost::asio::streambuf& message) override
    {

        std::istringstream input;
        input.str( std::string( (const char*)message.data().data(), message.size() ) );

        std::string command;
        std::getline( input, command, ';');
        
        if ( command == JOIN_TO_CHAT_CMD )
        {
            std::cout << "Hello! You've joined to the chat. Please, enter your name";

            std::string username;
            input >> username;

            Client newClient(*this, &client);
        }
        else if ( command == MESSAGE_TO_ALL_CMD )
        {
            std::string message;
            std::getline(input, message);

            std::shared_ptr<boost::asio::streambuf> wrStreambuf
                = std::make_shared<boost::asio::streambuf>();
            std::ostream os(&(*wrStreambuf));
            os << MESSAGE_FROM_CMD ";" << message;
        }
    }
};
