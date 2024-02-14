#include <QApplication>

#include "ChatInterfaces.h"
#include "ChatModel.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::thread dbThread(
            []
            {
                gDatabaseIoContext.run();
            });
    std::thread serverThread(
        []
        {
            ChatModel chat;

            auto server = createServer(gServerIoContext, chat, 1234);
            server->execute();
        });
    serverThread.join();
}
