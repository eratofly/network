#ifndef RDT_HEADER_H
#define RDT_HEADER_H

#include <cstdint>
#include <iostream>

const int MAX_DATA_SIZE = 1024;
const int PACKET_HEADER_SIZE = sizeof(uint32_t) * 2 + sizeof(uint16_t) * 2;

const uint16_t FLAG_DATA = 0x0;
const uint16_t FLAG_SYN  = 0x1;
const uint16_t FLAG_FIN  = 0x2;
const uint16_t FLAG_ACK  = 0x4;

struct Packet {
    uint32_t seqNum;
    uint32_t ackNum;
    uint16_t flags;
    uint16_t dataSize;
    char     data[MAX_DATA_SIZE];
};

struct RdtStats {
    uint64_t totalBytesSent = 0;
    uint64_t totalPacketsSent = 0;
    uint64_t retransmissions = 0;
    uint64_t droppedPackets = 0; // Для симуляции
};

#endif