#include "RdtSender.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <cmath>

using namespace std::chrono;

RdtSender::RdtSender(const std::string& destIp, int destPort, bool debug)
    : m_debug(debug)
{
    m_socket.SetDestination(destIp, destPort);
    m_socket.SetRecvTimeout(5);
}

void RdtSender::Log(const std::string& msg) {
    if (m_debug) {
        std::cout << "[Sender] " << msg << std::endl;
    }
}

void RdtSender::PrintProgress(float progress) {
    int barWidth = 50;

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
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
        std::memset(&pkt, 0, sizeof(pkt));
        pkt.seqNum = seq;
        pkt.flags = FLAG_DATA;
        file.read(pkt.data, MAX_DATA_SIZE);
        pkt.dataSize = file.gcount();
        if (pkt.dataSize > 0) {
            pkt.checksum = calculateChecksum(pkt);
            m_packets.push_back(pkt);
            seq++;
        }
    }

    Packet finPkt;
    std::memset(&finPkt, 0, sizeof(finPkt));
    finPkt.seqNum = seq;
    finPkt.flags = FLAG_FIN;
    finPkt.dataSize = 0;
    finPkt.checksum = calculateChecksum(finPkt);
    m_packets.push_back(finPkt);

    std::cout << "File read. Total packets: " << m_packets.size() << std::endl;
}

void RdtSender::SendFile(const std::string& filename) {
    PreparePackets(filename);

    m_base = 0;
    m_nextSeqNum = 0;
    int totalPackets = m_packets.size();

    RdtStats stats;
    int consecutiveRetries = 0;

    auto timerStart = high_resolution_clock::now();
    bool timerRunning = false;

    if (!m_debug) PrintProgress(0.0);

    while (m_base < totalPackets) {

        while (m_nextSeqNum < m_base + m_windowSize && m_nextSeqNum < totalPackets) {
            Packet& pkt = m_packets[m_nextSeqNum];
            Log("Sending packet SEQ: " + std::to_string(pkt.seqNum));

            if (m_socket.SendUnreliable(pkt, m_lossRate, m_maxDelay)) {
                stats.totalPacketsSent++;
                stats.totalBytesSent += pkt.dataSize + PACKET_HEADER_SIZE;
            } else {
                stats.droppedPackets++;
            }

            if (m_base == m_nextSeqNum) {
                timerStart = high_resolution_clock::now();
                timerRunning = true;
            }
            m_nextSeqNum++;
        }

        Packet ackPkt;
        int bytes = m_socket.Recv(ackPkt);

        if (bytes > 0) {
            if (calculateChecksum(ackPkt) == ackPkt.checksum && (ackPkt.flags & FLAG_ACK)) {
                Log("Received ACK: " + std::to_string(ackPkt.ackNum));

                if (ackPkt.ackNum >= m_base) {
                    m_base = ackPkt.ackNum + 1;
                    consecutiveRetries = 0;
                    Log("Window moved. Base is now: " + std::to_string(m_base));

                    if (!m_debug) {
                        PrintProgress((float)m_base / totalPackets);
                    }

                    if (m_base == m_nextSeqNum) {
                        timerRunning = false;
                    } else {
                        timerStart = high_resolution_clock::now();
                        timerRunning = true;
                    }
                }
            }
        }

        if (timerRunning) {
            auto now = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(now - timerStart).count();
            if (duration > m_timeoutMs) {
                Log("TIMEOUT on SEQ " + std::to_string(m_base));

                consecutiveRetries++;
                if (consecutiveRetries >= m_maxRetries) {
                    if (!m_debug) std::cout << std::endl;
                    std::cerr << "Max retries exceeded. Connection lost." << std::endl;
                    break;
                }

                Log("Retransmitting window.");
                stats.retransmissions++;
                m_nextSeqNum = m_base;
                timerRunning = false;
            }
        }
    }

    if (!m_debug) std::cout << std::endl;

    std::cout << "Transfer finished." << std::endl;
    std::cout << "---------------- Statistics ----------------" << std::endl;
    std::cout << "Total Bytes Sent: " << stats.totalBytesSent << std::endl;
    std::cout << "Total Packets Sent: " << stats.totalPacketsSent << std::endl;
    std::cout << "Retransmissions: " << stats.retransmissions << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
}