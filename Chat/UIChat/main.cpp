#include "mainwindow.h"
#include "Client/ChatClient.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        secondClient = true;
    }
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}