cmake_minimum_required(VERSION 3.5)

project(Chat LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "/home/vitaliykolbasiuk/Qt/6.6.0/gcc_64/lib/cmake")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS Sql)

include_directories(UIChat)
include_directories(SYSTEM UIChat/cereal/include)

if (APPLE)
    add_definitions(-DBOOST_ASIO_HAS_STD_INVOKE_RESULT)
    include_directories(SYSTEM /usr/local/include)
    set(CMAKE_PREFIX_PATH "~/Qt/6.4.0/macos")
    set(CMAKE_PREFIX_PATH "~/Qt/6.4.0/macos")
    set(BOOST_INCLUDE_DIR "/usr/local/include")
    set(BOOST_LIB_DIR "/usr/local/lib")

    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    set(OPENSSL_LIBRARIES "/usr/local/opt/openssl/lib")

    # for -lcrypto (I do not know who uses it!)
    link_directories(/usr/local/opt/openssl/lib)
endif()

if (WIN32)
    include_directories(SYSTEM ../../../Boost/boost_1_83_0/)
    set(CMAKE_PREFIX_PATH "C:/Qt6/6.6.0/msvc2019_64/lib/cmake")
    # for -lcrypto (I do not know who uses it!)
    #link_directories(/usr/local/opt/openssl/lib)
endif()


#add_executable(ThreadTest
#  main.cpp
#  #Log.h
#  Client/ChatClient.h
#  Client/ClientInterfaces.h
#  Client/TcpClient.h
#  BoostClientServer/Chat.h
#  BoostClientServer/Server.cpp
#  BoostClientServer/ChatInterfaces.h
#)
add_executable(StartServer
    #Log.h
    BoostClientServer/server_main.cpp
    BoostClientServer/Chat.h
    BoostClientServer/Server.cpp
    BoostClientServer/ChatInterfaces.h
    BoostClientServer/ChatDatabase.cpp
    BoostClientServer/ServerSession.h
    UIChat/ed25519/src/add_scalar.cpp
    UIChat/ed25519/src/fe.cpp
    UIChat/ed25519/src/ge.cpp
    UIChat/ed25519/src/key_exchange.cpp
    UIChat/ed25519/src/keypair.cpp
    UIChat/ed25519/src/sc.cpp
    UIChat/ed25519/src/seed.cpp
    UIChat/ed25519/src/sha512.cpp
    UIChat/ed25519/src/sign.cpp
    UIChat/ed25519/src/verify.cpp
)

add_executable(StartClient
        UIChat/main.cpp
        UIChat/mainwindow.cpp
        UIChat/mainwindow.h
        UIChat/mainwindow.ui
        UIChat/Settings.cpp
        UIChat/Settings.h
        UIChat/Utils.h
        UIChat/Client/ChatClient.h
        UIChat/Client/ClientInterfaces.h
        UIChat/Client/TcpClient.h
        UIChat/Protocol.h
        UIChat/CreateChatRoom.h
        UIChat/CreateChatRoom.cpp
        UIChat/CreateChatRoom.ui
        UIChat/ed25519/src/add_scalar.cpp
        UIChat/ed25519/src/fe.cpp
        UIChat/ed25519/src/ge.cpp
        UIChat/ed25519/src/key_exchange.cpp
        UIChat/ed25519/src/keypair.cpp
        UIChat/ed25519/src/sc.cpp
        UIChat/ed25519/src/seed.cpp
        UIChat/ed25519/src/sha512.cpp
        UIChat/ed25519/src/sign.cpp
        UIChat/ed25519/src/verify.cpp
        UIChat/SettingsDialog.cpp
        UIChat/SettingsDialog.h
        UIChat/SettingsDialog.ui
        UIChat/ChatRoomConnect.cpp
        UIChat/ChatRoomConnect.h
        UIChat/ChatRoomConnect.ui
        UIChat/Types.h
)

add_custom_target(StartClient2
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/StartClient 2
        DEPENDS StartClient
)

set_target_properties(StartServer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
#add_executable(StartClient
#    #Log.h
#    Client/client_main.cpp
#    Client/Terminal.h
#    Client/ChatClient.h
#    Client/ClientInterfaces.h
#    Client/TcpClient.h
#)
target_link_libraries(StartServer PRIVATE Qt6::Sql Qt6::Core Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(StartClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

#target_link_libraries(ThreadTest Qt${QT_VERSION_MAJOR}::Core)

#include(GNUInstallDirs)
#install(TARGETS ThreadTest
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)
