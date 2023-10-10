cmake_minimum_required(VERSION 3.5)

project(Chat LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

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

add_executable(ThreadTest
  main.cpp
  #Log.h
  Client/ChatClient.h
  Client/ClientInterfaces.h
  Client/TcpClient.h
  BoostClientServer/Chat.h
  BoostClientServer/Server.h
  BoostClientServer/ChatInterfaces.h
)
add_executable(StartServer
    #Log.h
    BoostClientServer/server_main.cpp
    BoostClientServer/Chat.h
    BoostClientServer/Server.h
    BoostClientServer/ChatInterfaces.h
)

add_executable(StartClient
    #Log.h
    Client/client_main.cpp
    Client/Terminal.h
    Client/ChatClient.h
    Client/ClientInterfaces.h
    Client/TcpClient.h
)

#target_link_libraries(ThreadTest Qt${QT_VERSION_MAJOR}::Core)

#include(GNUInstallDirs)
#install(TARGETS ThreadTest
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)
