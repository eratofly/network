cmake_minimum_required(VERSION 3.31)
project(proxy)

set(CMAKE_CXX_STANDARD 14)

add_executable(proxy
        proxy_server.cpp)
