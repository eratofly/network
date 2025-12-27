#include "RdtReceiver.h"
#include <fstream>
#include <iostream>
#include <unistd.h>

RdtReceiver::RdtReceiver(int port, bool debug) : m_debug(debug) {
    if (!m_socket.Bind(port)) {
        std::cerr << "Failed to bind port " << port << std::endl;
        exit(1);
    }
    std::cout << "Receiver listening on port " << port << std::endl;
}

void RdtReceiver::Log(const std::string& msg) {
    if (m_debug) {
        std::cout << "[Receiver] " << msg << std::endl;
    }
}

void RdtReceiver::SendAck(uint32_t ackNum, sockaddr_in& dest) {
    Packet ack;
    ack.flags = FLAG_ACK;
    ack.ackNum = ackNum;
    ack.dataSize = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr*)&dest, sizeof(dest));
    close(sock);
}

void RdtReceiver::ReceiveFile(const std::string& outputFilename) {
    std::ofstream file(outputFilename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file for writing" << std::endl;
        return;
    }

    sockaddr_in senderAddr;
    m_expectedSeqNum = 0;

    while (true) {
        Packet pkt;
        int bytes = m_socket.Recv(pkt, &senderAddr);

        if (bytes > 0) {
            if (pkt.flags & FLAG_FIN) {
                Log("Received FIN. Sending ACK and closing.");
                for(int i=0; i<3; i++) SendAck(pkt.seqNum, senderAddr);
                break;
            }

            if (pkt.seqNum == m_expectedSeqNum) {
                Log("Packet " + std::to_string(pkt.seqNum) + " received correctly.");

                if (pkt.dataSize > 0) {
                    file.write(pkt.data, pkt.dataSize);
                }

                SendAck(m_expectedSeqNum, senderAddr);
                m_expectedSeqNum++;
            } else {
                Log("Out of order packet: " + std::to_string(pkt.seqNum) +
                    " Expected: " + std::to_string(m_expectedSeqNum));

                if (m_expectedSeqNum > 0) {
                    SendAck(m_expectedSeqNum - 1, senderAddr);
                } else {

                }
            }
        }
    }
    file.close();
    std::cout << "File received successfully." << std::endl;
}