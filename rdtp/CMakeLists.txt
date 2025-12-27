cmake_minimum_required(VERSION 3.31)
project(rdtp_project)

set(CMAKE_CXX_STANDARD 14)

add_executable(rdt_sender
        sender_main.cpp
        RdtSender.cpp
        RdtSender.h
        UdpSocket.cpp
        UdpSocket.h
        RdtHeader.h
)

add_executable(rdt_receiver
        receiver_main.cpp
        RdtReceiver.cpp
        RdtReceiver.h
        UdpSocket.cpp
        UdpSocket.h
        RdtHeader.h
)