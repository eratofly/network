#ifndef RDT_SENDER_H
#define RDT_SENDER_H

#include <vector>
#include <string>
#include "UdpSocket.h"
#include "RdtHeader.h"

class RdtSender {
public:
    RdtSender(const std::string& destIp, int destPort, bool debug);
    void SendFile(const std::string& filename);

private:
    void PreparePackets(const std::string& filename);
    void Log(const std::string& msg);

    UdpSocket m_socket;
    std::vector<Packet> m_packets;
    bool m_debug;

    int m_windowSize = 5;
    int m_base = 0;
    int m_nextSeqNum = 0;
    int m_timeoutMs = 500;

    double m_lossRate = 0.2;
    int m_maxDelay = 50;
};

#endif