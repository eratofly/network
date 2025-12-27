#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H

#include <cstdint>
#include <string>
#include <vector>

constexpr int DNS_PORT = 53;
constexpr int BUFFER_SIZE = 65536;
constexpr int TIMEOUT_SEC = 3;

constexpr int TYPE_A = 1;
constexpr int TYPE_NS = 2;
constexpr int TYPE_CNAME = 5;
constexpr int TYPE_AAAA = 28;

#pragma pack(push, 1)
struct DnsHeader
{
    uint16_t id;
    uint16_t flags;
    uint16_t qCount;
    uint16_t ansCount;
    uint16_t authCount;
    uint16_t addCount;
};
#pragma pack(pop)

struct ParsedRecord
{
    std::string name;
    uint16_t type;
    uint16_t dataClass;
    uint32_t ttl;
    std::string rdata;
    std::vector<uint8_t> rawData;
};

class DnsResolver
{
public:
    explicit DnsResolver(bool debug);
    ~DnsResolver();

    void Resolve(const std::string &domain, const std::string &recordTypeStr);

private:
    bool m_debugMode;
    std::vector<std::string> m_rootServers;

    void Log(const std::string &message) const;
    int GetRecordTypeFromString(const std::string &typeStr);

    void EncodeDomainName(const std::string &host, std::vector<uint8_t> &buffer);
    std::string DecodeDomainName(const uint8_t *reader, const uint8_t *bufferStart, int *bytesRead);

    int CreateSocket();
    std::vector<uint8_t> BuildQueryPacket(const std::string &domain, int recordType);
    bool PerformQuery(const std::string &serverIp, const std::string &domain, int recordType,
                      std::vector<uint8_t> &responseBuffer);

    void ParseResponse(const std::vector<uint8_t> &buffer, std::vector<ParsedRecord> &answers,
                       std::vector<ParsedRecord> &authorities, std::vector<ParsedRecord> &additionals);
    const uint8_t *ParseSection(int count, const uint8_t *reader, const uint8_t *bufferStart,
                                std::vector<ParsedRecord> &outVector);

    std::string FindGlueIp(const std::string &nsName, const std::vector<ParsedRecord> &additionals);
};

#endif // DNS_RESOLVER_H
