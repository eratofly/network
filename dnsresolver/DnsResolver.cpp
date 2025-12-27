#include "DnsResolver.h"
#include <arpa/inet.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


void PrintHex(const std::vector<uint8_t> &buffer)
{
    std::cout << "Hex Dump (" << buffer.size() << " bytes):" << std::endl;
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

bool IsEqualIgnoreCase(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (std::tolower((unsigned char) a[i]) != std::tolower((unsigned char) b[i]))
            return false;
    }
    return true;
}

DnsResolver::DnsResolver(bool debug) : m_debugMode(debug)
{
    m_rootServers.emplace_back("198.41.0.4");
    m_rootServers.emplace_back("199.9.14.201");
    m_rootServers.emplace_back("192.33.4.12");
}

DnsResolver::~DnsResolver() = default;

void DnsResolver::Log(const std::string &message) const
{
    if (m_debugMode)
    {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

int DnsResolver::GetRecordTypeFromString(const std::string &typeStr)
{
    if (typeStr == "AAAA")
        return TYPE_AAAA;
    if (typeStr == "NS")
        return TYPE_NS;
    return TYPE_A;
}

void DnsResolver::EncodeDomainName(const std::string &host, std::vector<uint8_t> &buffer)
{
    std::string part;
    size_t start = 0;
    size_t end = host.find('.');

    while (end != std::string::npos)
    {
        part = host.substr(start, end - start);
        buffer.push_back(static_cast<uint8_t>(part.length()));
        for (char c: part)
            buffer.push_back(static_cast<uint8_t>(c));
        start = end + 1;
        end = host.find('.', start);
    }
    part = host.substr(start);
    if (!part.empty())
    {
        buffer.push_back(static_cast<uint8_t>(part.length()));
        for (char c: part)
            buffer.push_back(c);
    }
    buffer.push_back(0);
}

std::string DnsResolver::DecodeDomainName(const uint8_t *reader, const uint8_t *bufferStart, int *bytesRead)
{
    std::string name;
    bool jumped = false;
    *bytesRead = 0;
    const uint8_t *current = reader;

    while (*current != 0)
    {
        if (*current >= 192)
        {
            int offset = (*current) * 256 + *(current + 1) - 49152;
            current = bufferStart + offset - 1;
            jumped = true;
        } else
        {
            name.append((char *) (current + 1), *current);
            name.append(".");
        }
        current += *current + 1;
    }

    if (jumped)
    {
        const uint8_t *tmp = reader;
        while (*tmp != 0 && *tmp < 192)
            tmp += *tmp + 1;
        *bytesRead = (tmp - reader) + (*tmp >= 192 ? 2 : 1);
    } else
    {
        *bytesRead = (current - reader) + 1;
    }

    if (!name.empty() && name.back() == '.')
        name.pop_back();
    return name;
}

int DnsResolver::CreateSocket()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        return -1;

    struct timeval tv{};
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    return sockfd;
}

std::vector<uint8_t> DnsResolver::BuildQueryPacket(const std::string &domain, int recordType)
{
    std::vector<uint8_t> packet;

    DnsHeader header;
    memset(&header, 0, sizeof(header));

    header.id = (uint16_t) htons(getpid() & 0xFFFF);

    header.flags = htons(0x0000);
    header.qCount = htons(1);

    uint8_t *headerPtr = (uint8_t *) &header;
    for (size_t i = 0; i < sizeof(header); i++)
        packet.push_back(headerPtr[i]);

    EncodeDomainName(domain, packet);

    uint16_t qtype = htons(recordType);
    uint16_t qclass = htons(1);

    packet.push_back((qtype >> 8) & 0xFF);
    packet.push_back(qtype & 0xFF);
    packet.push_back((qclass >> 8) & 0xFF);
    packet.push_back(qclass & 0xFF);

    return packet;
}

bool DnsResolver::PerformQuery(const std::string &serverIp, const std::string &domain, int recordType,
                               std::vector<uint8_t> &responseBuffer)
{
    int sockfd = CreateSocket();
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return false;
    }

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DNS_PORT);
    dest.sin_addr.s_addr = inet_addr(serverIp.c_str());

    std::vector<uint8_t> queryPacket = BuildQueryPacket(domain, recordType);

    if (m_debugMode)
    {
        std::cout << ">>> Sending Query to " << serverIp << " <<<" << std::endl;
        PrintHex(queryPacket);
    }

    if (sendto(sockfd, queryPacket.data(), queryPacket.size(), 0, (struct sockaddr *) &dest, sizeof(dest)) < 0)
    {
        Log("Failed to send packet");
        close(sockfd);
        return false;
    }

    uint8_t buf[BUFFER_SIZE];
    struct sockaddr_in sender{};
    socklen_t senderLen = sizeof(sender);

    int bytesRead = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &sender, &senderLen);
    close(sockfd);

    if (bytesRead < 0)
    {
        Log("Timeout or error receiving data from " + serverIp);
        return false;
    }

    responseBuffer.assign(buf, buf + bytesRead);
    return true;
}

const uint8_t *DnsResolver::ParseSection(int count, const uint8_t *reader, const uint8_t *bufferStart,
                                         std::vector<ParsedRecord> &outVector)
{
    for (int i = 0; i < count; i++)
    {
        ParsedRecord rec;
        int bytesSkipped = 0;

        rec.name = DecodeDomainName(reader, bufferStart, &bytesSkipped);
        reader += bytesSkipped;

        rec.type = ntohs(*(uint16_t *) reader);
        reader += 2;
        rec.dataClass = ntohs(*(uint16_t *) reader);
        reader += 2;
        rec.ttl = ntohl(*(uint32_t *) reader);
        reader += 4;
        uint16_t dataLen = ntohs(*(uint16_t *) reader);
        reader += 2;

        rec.rawData.assign(reader, reader + dataLen);

        if (rec.type == TYPE_A && dataLen == 4)
        {
            struct sockaddr_in ip{};
            ip.sin_addr.s_addr = *(long *) reader;
            rec.rdata = inet_ntoa(ip.sin_addr);
        } else if (rec.type == TYPE_AAAA)
        {
            char str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, reader, str, INET6_ADDRSTRLEN);
            rec.rdata = std::string(str);
        } else if (rec.type == TYPE_NS || rec.type == TYPE_CNAME)
        {
            int nameBytes = 0;
            rec.rdata = DecodeDomainName(reader, bufferStart, &nameBytes);
        }

        reader += dataLen;
        outVector.push_back(rec);
    }
    return reader;
}

void DnsResolver::ParseResponse(const std::vector<uint8_t> &buffer, std::vector<ParsedRecord> &answers,
                                std::vector<ParsedRecord> &authorities, std::vector<ParsedRecord> &additionals)
{

    if (buffer.size() < sizeof(DnsHeader))
        return;

    if (m_debugMode)
    {
        std::cout << "<<< Received Response <<<" << std::endl;
        PrintHex(buffer);
    }

    const DnsHeader *header = (DnsHeader *) buffer.data();

    uint16_t qCount = ntohs(header->qCount);
    uint16_t ansCount = ntohs(header->ansCount);
    uint16_t authCount = ntohs(header->authCount);
    uint16_t addCount = ntohs(header->addCount);
    uint16_t flags = ntohs(header->flags);
    uint16_t rcode = flags & 0x000F;

    if (m_debugMode)
    {
        std::cout << "Header Info -> Answers: " << ansCount << ", Auth: " << authCount << ", Add: " << addCount
                  << ", RCODE: " << rcode << std::endl;
    }

    if (rcode != 0)
    {
        std::cerr << "Server returned error RCODE: " << rcode << std::endl;
        return;
    }

    const uint8_t *reader = buffer.data() + sizeof(DnsHeader);
    const uint8_t *bufStart = buffer.data();

    for (int i = 0; i < qCount; i++)
    {
        int bytes = 0;
        DecodeDomainName(reader, bufStart, &bytes);
        reader += bytes + 4;
    }

    reader = ParseSection(ansCount, reader, bufStart, answers);
    reader = ParseSection(authCount, reader, bufStart, authorities);
    reader = ParseSection(addCount, reader, bufStart, additionals);
}

std::string DnsResolver::FindGlueIp(const std::string &nsName, const std::vector<ParsedRecord> &additionals)
{
    if (m_debugMode)
        std::cout << "[Glue Search] Looking for IP for: " << nsName << std::endl;

    for (const auto &rec: additionals)
    {
        if (rec.type == TYPE_A)
        {
            if (IsEqualIgnoreCase(rec.name, nsName))
            {
                if (m_debugMode)
                    std::cout << "[Glue Search] Found: " << rec.rdata << std::endl;
                return rec.rdata;
            }
        }
    }
    return "";
}

void DnsResolver::Resolve(const std::string &domain, const std::string &recordTypeStr)
{
    int targetType = GetRecordTypeFromString(recordTypeStr);

    std::string currentNsIp = m_rootServers[rand() % m_rootServers.size()];
    std::string currentDomain = domain;
    int iterations = 0;
    const int MAX_ITERATIONS = 20;

    Log("Starting Iterative Resolution for " + currentDomain + " (" + recordTypeStr + ")");

    while (iterations++ < MAX_ITERATIONS)
    {
        Log("Querying Server: " + currentNsIp);

        std::vector<uint8_t> response;
        if (!PerformQuery(currentNsIp, currentDomain, targetType, response))
        {
            std::cerr << "Communication error with " << currentNsIp << std::endl;
            return;
        }

        std::vector<ParsedRecord> answers, authorities, additionals;
        ParseResponse(response, answers, authorities, additionals);

        bool foundAnswer = false;
        for (const auto &rec: answers)
        {
            if (rec.type == targetType)
            {
                Log("Answer found!");
                std::cout << "Result: " << rec.rdata << std::endl;
                foundAnswer = true;
            } else if (rec.type == TYPE_CNAME)
            {
                Log("Found CNAME: " + rec.rdata);
                currentDomain = rec.rdata;
                currentNsIp = m_rootServers[0];
                foundAnswer = true;
                break;
            }
        }

        if (foundAnswer)
        {
            if (currentDomain == domain && targetType != TYPE_CNAME)
                return;
            if (currentDomain != domain)
                continue;
        }

        if (!authorities.empty())
        {
            bool foundGlue = false;
            for (const auto &nsRec: authorities)
            {
                if (nsRec.type == TYPE_NS)
                {
                    std::string glueIp = FindGlueIp(nsRec.rdata, additionals);
                    if (!glueIp.empty())
                    {
                        currentNsIp = glueIp;
                        foundGlue = true;
                        break;
                    }
                }
            }

            if (!foundGlue)
            {
                Log("No Glue found in Additional section. Trying to resolve NS name recursively is not implemented.");
                std::cerr << "Resolution Dead End (Missing Glue)." << std::endl;
                return;
            }
        } else
        {
            Log("No Authority records. Host not found.");
            return;
        }
    }
    std::cerr << "Too many iterations." << std::endl;
}
