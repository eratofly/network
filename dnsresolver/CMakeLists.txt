cmake_minimum_required(VERSION 3.31)
project(dnsresolver)

set(CMAKE_CXX_STANDARD 14)

add_executable(dnsresolver
        main.cpp
        DnsResolver.cpp
        DnsResolver.h)
