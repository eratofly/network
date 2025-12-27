#include "RdtSender.h"
#include <fstream>
#include <iostream>
#include <chrono>

using namespace std::chrono;

RdtSender::RdtSender(const std::string& destIp, int destPort, bool debug)
    : m_debug(debug)
{
    m_socket.SetDestination(destIp, destPort);
    m_socket.SetRecvTimeout(10);
}

void RdtSender::Log(const std::string& msg) {
    if (m_debug) {
        std::cout << "[Sender] " << msg << std::endl;
    }
}

void RdtSender::PreparePackets(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        exit(1);
    }

    int seq = 0;
    while (!file.eof()) {
        Packet pkt;
        pkt.seqNum = seq;
        pkt.flags = FLAG_DATA;
        file.read(pkt.data, MAX_DATA_SIZE);
        pkt.dataSize = file.gcount();
        if (pkt.dataSize > 0) {
            m_packets.push_back(pkt);
            seq++;
        }
    }

    Packet finPkt;
    finPkt.seqNum = seq;
    finPkt.flags = FLAG_FIN;
    finPkt.dataSize = 0;
    m_packets.push_back(finPkt);

    std::cout << "File read. Total packets: " << m_packets.size() << std::endl;
}

void RdtSender::SendFile(const std::string& filename) {
    PreparePackets(filename);

    m_base = 0;
    m_nextSeqNum = 0;
    int totalPackets = m_packets.size();

    auto timerStart = high_resolution_clock::now();
    bool timerRunning = false;

    while (m_base < totalPackets) {
        while (m_nextSeqNum < m_base + m_windowSize && m_nextSeqNum < totalPackets) {
            Packet& pkt = m_packets[m_nextSeqNum];
            Log("Sending packet SEQ: " + std::to_string(pkt.seqNum));
            m_socket.SendUnreliable(pkt, m_lossRate, m_maxDelay);

            if (m_base == m_nextSeqNum) {
                timerStart = high_resolution_clock::now();
                timerRunning = true;
            }
            m_nextSeqNum++;
        }

        Packet ackPkt;
        int bytes = m_socket.Recv(ackPkt);
        if (bytes > 0 && (ackPkt.flags & FLAG_ACK)) {
            Log("Received ACK: " + std::to_string(ackPkt.ackNum));

            if (ackPkt.ackNum >= m_base) {
                m_base = ackPkt.ackNum + 1;
                Log("Window moved. Base is now: " + std::to_string(m_base));

                if (m_base == m_nextSeqNum) {
                    timerRunning = false;
                } else {
                    timerStart = high_resolution_clock::now();
                    timerRunning = true;
                }
            }
        }

        if (timerRunning) {
            auto now = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(now - timerStart).count();
            if (duration > m_timeoutMs) {
                Log("TIMEOUT on SEQ " + std::to_string(m_base) + ". Retransmitting window.");
                m_nextSeqNum = m_base;
                timerRunning = false;
            }
        }
    }
    std::cout << "Transfer complete." << std::endl;
}