#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <netpacket/packet.h>
#include <net/if.h>

typedef struct 
{
	uint8_t destinationMac[6];
	uint8_t sourceMac[6];
	uint16_t etherType;
}__attribute__((packed)) EtherHeader;

typedef struct {
    uint8_t headerLenAndIpVersion;
    uint8_t typeOfService;
    uint16_t totalLength;
    uint16_t ipId;
    uint16_t fragmentOffset;
    uint8_t timeToLive;
    uint8_t protocol;
    uint16_t headerChecksum;
    uint32_t ipSrc;
    uint32_t ipDst;
}__attribute__((packed)) IPv4Header;

typedef struct
{
    uint16_t portSrc;
    uint16_t portDst;
    uint16_t length;
    uint16_t headerChecksum;
}__attribute__((packed)) UdpHeader;

uint8_t udp_packet[1024] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x2e, 0x7c, 0xbc, 0x40, 0x00, 0x40, 0x11, 0xc0, 0x00, 0x7f, 0x00, 0x00, 0x01, 0x7f, 0x00,
    0x00, 0x01, 0xe3, 0x82, 0x1f, 0x90, 0x00, 0x1a, 0xfe, 0x2d, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c,
    0x20, 0x55, 0x44, 0x50, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x21
    };

// function created by Copilot
// IP 헤더의 체크섬은 모든 16비트 워드를 더한 후 1의 보수를 계산한다.
uint16_t calculate_ip_checksum(const void* vdata, size_t length) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);
    uint32_t acc = 0;

    for (size_t i = 0; i + 1 < length; i += 2) 
    {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
    }

    if (length & 1) 
    {
        acc += data[length - 1] << 8;
    }

    while (acc >> 16) 
    {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }

    return htons(~acc);
}

int main(int argc, char* argv[]) 
{

    // Raw 소켓 생성
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    else
    {
        std::cout << "Socket created successfully" << std::endl;
    }

    // 링크 계층(Layer2) 패킷을 다루기 위해 소켓을 바인딩
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    // "lo" 인터페이스를 사용, 다른 인터페이스 사용 시 자신의 환경의 인터페이스 명이 어떻게 되어있는지 확인 후 변경(ex: eth0)
    sll.sll_ifindex = if_nametoindex("lo");

    if (bind(sock, (struct sockaddr*)&sll, sizeof(sll)) < 0) 
    {
        perror("bind");
        close(sock);
        return 1;
    }

    // 원본 패킷에 각 헤더의 시작 주소를 설정하여 값을 수정 가능하도록 구현
    EtherHeader* etherHeader = reinterpret_cast<EtherHeader*>(udp_packet);
    IPv4Header* ipHeader = reinterpret_cast<IPv4Header*>(udp_packet + sizeof(EtherHeader));
    UdpHeader* udpHeader = reinterpret_cast<UdpHeader*>(udp_packet + sizeof(EtherHeader) + sizeof(IPv4Header));

    // IP 헤더의 source ip를 변경
    ipHeader->ipSrc = inet_addr("127.0.0.2");

    // IP 헤더의 체크섬 계산을 다시 해줘야 한다.
    ipHeader->headerChecksum = 0; // 체크섬 계산을 다시 하기위해 기존 체크섬 값은 0으로 초기화
    ipHeader->headerChecksum = calculate_ip_checksum(ipHeader, sizeof(IPv4Header));

    // send할 패킷의 길이를 구하기 위해 IP 헤더의 totalLength에 Ethernet 헤더의 총 크기를 더한다.
    uint16_t sendPacketLength = ntohs(ipHeader->totalLength) + sizeof(EtherHeader);

    // 패킷 전송
    if (sendto(sock, udp_packet, sendPacketLength, 0, (struct sockaddr*)&sll, sizeof(sll)) < 0) 
    {
        perror("sendto");
        close(sock);
        return 1;
    }
    else
    {
        std::cout << "Packet sent successfully" << std::endl;
    }

    close(sock);
    return 0;
}