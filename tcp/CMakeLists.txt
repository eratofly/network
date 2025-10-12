cmake_minimum_required(VERSION 3.31)
project(tcp)

set(CMAKE_CXX_STANDARD 14)

add_executable(tcp
        SocketHandler.cpp
        Client.h
        Client.cpp
        Server.h
        Server.cpp
        main.cpp)
