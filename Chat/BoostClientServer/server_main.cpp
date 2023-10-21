#include <iostream>
#include <QApplication>

#include "ChatInterfaces.h"
#include "Chat.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::cout << "Server started" << std::endl;
    IChatDatabase* database = createDatabase();
    database->test();
    return 0;
    // server
    std::thread serverThread(
        []
        {
            io_context serverIoContext;
            Chat chat;

            IServer* server = createServer(serverIoContext, chat, 1234);
            server->execute();
        });
    serverThread.join();
    std::cout << "Server ended" << std::endl;
}
