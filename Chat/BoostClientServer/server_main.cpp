#include <iostream>
#include <QApplication>

#include "ChatInterfaces.h"
#include "Chat.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

//    std::cout << "Server started" << std::endl;
//    return 0;
    // server

    std::thread dbThread(
            []
            {
                gDatabaseIoContext.run();
            });
    std::thread serverThread(
        []
        {
            IChatDatabase* database = createDatabase();
            database->test();
            Chat chat(*database);

            IServer* server = createServer(gServerIoContext, chat, 1234);
            server->execute();
        });
    serverThread.join();
    std::cout << "Server ended" << std::endl;
}
