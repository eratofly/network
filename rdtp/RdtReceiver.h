#ifndef RDT_RECEIVER_H
#define RDT_RECEIVER_H

#include "UdpSocket.h"
#include <string>

class RdtReceiver {
public:
    RdtReceiver(int port, bool debug);
    void ReceiveFile(const std::string& outputFilename);

private:
    void SendAck(uint32_t ackNum, sockaddr_in& dest);
    void Log(const std::string& msg);

    UdpSocket m_socket;
    bool m_debug;
    uint32_t m_expectedSeqNum = 0;
};

#endif