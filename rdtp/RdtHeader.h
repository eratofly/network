#ifndef RDT_HEADER_H
#define RDT_HEADER_H

#include <cstdint>
#include <iostream>
#include <cstring>

const int MAX_DATA_SIZE = 1024;
const int PACKET_HEADER_SIZE = sizeof(uint32_t) * 2 + sizeof(uint16_t) * 3;

const uint16_t FLAG_DATA = 0x0;
const uint16_t FLAG_SYN  = 0x1;
const uint16_t FLAG_FIN  = 0x2;
const uint16_t FLAG_ACK  = 0x4;

struct Packet {
    uint32_t seqNum;
    uint32_t ackNum;
    uint16_t flags;
    uint16_t dataSize;
    uint16_t checksum;
    char     data[MAX_DATA_SIZE];
};

struct RdtStats {
    uint64_t totalBytesSent = 0;
    uint64_t totalPacketsSent = 0;
    uint64_t retransmissions = 0;
    uint64_t droppedPackets = 0;
};

inline uint16_t calculateChecksum(Packet& pkt) {
    uint16_t oldSum = pkt.checksum;
    pkt.checksum = 0;

    uint32_t sum = 0;

    uint16_t* headerPtr = (uint16_t*)&pkt;
    for(int i=0; i<6; i++) {
        sum += headerPtr[i];
        if(sum > 0xFFFF) sum = (sum & 0xFFFF) + 1;
    }

    uint16_t* dataPtr = (uint16_t*)pkt.data;
    for (int i = 0; i < pkt.dataSize / 2; ++i) {
        sum += dataPtr[i];
        if (sum > 0xFFFF) sum = (sum & 0xFFFF) + 1;
    }

    if (pkt.dataSize % 2 != 0) {
        sum += (uint8_t)pkt.data[pkt.dataSize - 1];
        if (sum > 0xFFFF) sum = (sum & 0xFFFF) + 1;
    }

    pkt.checksum = oldSum;
    return ~((uint16_t)sum);
}

#endif