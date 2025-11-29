cmake_minimum_required(VERSION 3.31)
project(webserver)

set(CMAKE_CXX_STANDARD 14)

add_executable(webserver
        Webserver.cpp
        Webserver.h
        main.cpp)
