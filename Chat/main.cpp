#include <QCoreApplication>
#include <iostream>

#include "BoostClientServer/ChatClient.h"
#include "BoostClientServer/ChatInterfaces.h"
#include "BoostClientServer/Server.h"
#include "BoostClientServer/TcpClient.h"
#include "Chat.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // server
    std::thread(
        []
        {
            io_context serverIoContext;
            Chat chat(serverIoContext);

            TcpServer server(serverIoContext, chat, 1234);
            server.execute();
        })
        .detach();

    // Client
    std::thread(
        []
        {
            ChatClient chatClient;

            io_context ioContext1;
            TcpClient client1(ioContext1, chatClient);
            chatClient.setTcpClient(&client1);
            client1.execute("127.0.0.1", 1234, JOIN_TO_CHAT_CMD ";001;1000;800;");

            ioContext1.run();
        })
        .detach();
    return a.exec();
}
