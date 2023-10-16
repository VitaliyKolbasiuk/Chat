#include <iostream>

#include "Client/ChatClient.h"
#include "Client/ClientInterfaces.h"
#include "Client/TcpClient.h"
#include "BoostClientServer/ChatInterfaces.h"
#include "BoostClientServer/Server.cpp"
#include "BoostClientServer/Chat.h"

int main(int argc, char *argv[])
{
    // server
    std::thread(
        []
        {
            io_context serverIoContext;
            Chat chat;

            IServer* server = createServer(serverIoContext, chat, 1234);
            server->execute();
        })
        .detach();

    // Client
//    std::thread(
//        []
//        {
//            ChatClient chatClient;

//            io_context ioContext1;
//            TcpClient client1(ioContext1, chatClient);
//            chatClient.setTcpClient(&client1);
//            client1.execute("127.0.0.1", 1234, JOIN_TO_CHAT_CMD ";001;1000;800;");

//            ioContext1.run();
//        })
//        .detach();
}
