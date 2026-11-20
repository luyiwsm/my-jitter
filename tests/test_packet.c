#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <pcap.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "packet.h"

#define ETH_LEN 14
#define IP_LEN 20
#define UDP_LEN 8

static void init_header(
        struct pcap_pkthdr *header,
        unsigned int caplen)
{
    memset(header, 0, sizeof(*header));

    header->caplen = caplen;
    header->len = caplen;
    header->ts.tv_sec = 100;
    header->ts.tv_usec = 0;
}


/*
 * Test 1:
 * Ethernet frame is shorter than an Ethernet header.
 */
static void test_short_ethernet(void)
{
    unsigned char packet[10] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: short Ethernet frame\n");
}


/*
 * Test 2:
 * Non-IPv4 Ethernet frame should be ignored.
 */
static void test_non_ipv4(void)
{
    unsigned char packet[ETH_LEN] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    eth->ether_type = htons(ETHERTYPE_ARP);

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: non-IPv4 frame\n");
}


/*
 * Test 3:
 * Ethernet header exists, but IPv4 header is incomplete.
 */
static void test_short_ip_header(void)
{
    unsigned char packet[ETH_LEN + 10] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    eth->ether_type = htons(ETHERTYPE_IP);

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: short IPv4 header\n");
}


/*
 * Test 4:
 * IPv4 IHL is invalid (< 20 bytes).
 */
static void test_invalid_ip_header_length(void)
{
    unsigned char packet[ETH_LEN + IP_LEN] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    struct ip *ip_header =
        (struct ip *)(packet + ETH_LEN);

    eth->ether_type = htons(ETHERTYPE_IP);

    ip_header->ip_hl = 4;
    ip_header->ip_v = 4;
    ip_header->ip_p = IPPROTO_UDP;

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: invalid IPv4 IHL\n");
}


/*
 * Test 5:
 * IPv4 header claims to be longer than
 * the captured packet.
 */
static void test_truncated_ip_header(void)
{
    unsigned char packet[ETH_LEN + IP_LEN] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    struct ip *ip_header =
        (struct ip *)(packet + ETH_LEN);

    eth->ether_type = htons(ETHERTYPE_IP);

    ip_header->ip_hl = 6;
    ip_header->ip_v = 4;
    ip_header->ip_p = IPPROTO_UDP;

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: truncated IPv4 header\n");
}


/*
 * Test 6:
 * UDP header is incomplete.
 */
static void test_short_udp_header(void)
{
    unsigned char packet[ETH_LEN + IP_LEN + 4] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    struct ip *ip_header =
        (struct ip *)(packet + ETH_LEN);

    eth->ether_type = htons(ETHERTYPE_IP);

    ip_header->ip_hl = 5;
    ip_header->ip_v = 4;
    ip_header->ip_p = IPPROTO_UDP;

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: short UDP header\n");
}


/*
 * Test 7:
 * TCP header is incomplete.
 */
static void test_short_tcp_header(void)
{
    unsigned char packet[ETH_LEN + IP_LEN + 10] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    struct ip *ip_header =
        (struct ip *)(packet + ETH_LEN);

    eth->ether_type = htons(ETHERTYPE_IP);

    ip_header->ip_hl = 5;
    ip_header->ip_v = 4;
    ip_header->ip_p = IPPROTO_TCP;

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: short TCP header\n");
}


/*
 * Test 8:
 * Unsupported IP protocol should be ignored.
 */
static void test_unsupported_protocol(void)
{
    unsigned char packet[ETH_LEN + IP_LEN] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    struct ip *ip_header =
        (struct ip *)(packet + ETH_LEN);

    eth->ether_type = htons(ETHERTYPE_IP);

    ip_header->ip_hl = 5;
    ip_header->ip_v = 4;
    ip_header->ip_p = 1; /* ICMP */

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == -1);

    printf("PASS: unsupported protocol\n");
}


/*
 * Test 9:
 * Valid UDP packet should parse successfully
 * with the correct 5-tuple.
 */
static void test_valid_udp(void)
{
    unsigned char packet[
        ETH_LEN + IP_LEN + UDP_LEN
    ] = {0};

    struct pcap_pkthdr header;
    packet_info_t info;

    struct ether_header *eth =
        (struct ether_header *)packet;

    struct ip *ip_header =
        (struct ip *)(packet + ETH_LEN);

    struct udphdr *udp_header =
        (struct udphdr *)(
            packet + ETH_LEN + IP_LEN
        );

    eth->ether_type = htons(ETHERTYPE_IP);

    ip_header->ip_hl = 5;
    ip_header->ip_v = 4;
    ip_header->ip_p = IPPROTO_UDP;

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &ip_header->ip_src
    );

    inet_pton(
        AF_INET,
        "127.0.0.2",
        &ip_header->ip_dst
    );

    udp_header->uh_sport = htons(5000);
    udp_header->uh_dport = htons(6000);

    init_header(&header, sizeof(packet));

    assert(parse_packet(&header, packet, &info) == 0);

    struct in_addr expected_src;
    struct in_addr expected_dst;

    inet_pton(AF_INET, "127.0.0.1", &expected_src);
    inet_pton(AF_INET, "127.0.0.2", &expected_dst);

    assert(info.src_ip == expected_src.s_addr);
    assert(info.dst_ip == expected_dst.s_addr);
    assert(info.src_port == 5000);
    assert(info.dst_port == 6000);
    assert(info.protocol == IPPROTO_UDP);
    assert(info.arrival_time == 100.0);

    printf("PASS: valid UDP packet\n");
}


/* 带 IPv4 选项的 TCP 包：检查传输层偏移和微秒时间戳。 */
static void test_valid_tcp_with_ip_options(void)
{
    unsigned char packet[ETH_LEN + 24 + sizeof(struct tcphdr)] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;
    struct ether_header *eth = (struct ether_header *)packet;
    struct ip *ip = (struct ip *)(packet + ETH_LEN);
    struct tcphdr *tcp = (struct tcphdr *)(packet + ETH_LEN + 24);

    eth->ether_type = htons(ETHERTYPE_IP);
    ip->ip_v = 4;
    ip->ip_hl = 6;
    ip->ip_p = IPPROTO_TCP;
    inet_pton(AF_INET, "192.0.2.1", &ip->ip_src);
    inet_pton(AF_INET, "192.0.2.2", &ip->ip_dst);
    tcp->th_sport = htons(12345);
    tcp->th_dport = htons(443);
    init_header(&header, sizeof packet);
    header.ts.tv_usec = 250000;

    assert(parse_packet(&header, packet, &info) == 0);
    assert(info.src_ip == ip->ip_src.s_addr);
    assert(info.dst_ip == ip->ip_dst.s_addr);
    assert(info.src_port == 12345 && info.dst_port == 443);
    assert(info.protocol == IPPROTO_TCP);
    assert(info.arrival_time == 100.25);

    header.caplen--;
    assert(parse_packet(&header, packet, &info) == -1);
    printf("PASS: TCP with IPv4 options and fractional timestamp\n");
}

/* 非首片 IPv4 分片不含 UDP/TCP 端口头，应直接拒绝。 */
static void test_ipv4_non_first_fragment(void)
{
    unsigned char packet[ETH_LEN + IP_LEN + sizeof(struct udphdr)] = {0};
    struct pcap_pkthdr header;
    packet_info_t info;
    struct ether_header *eth = (struct ether_header *)packet;
    struct ip *ip = (struct ip *)(packet + ETH_LEN);

    eth->ether_type = htons(ETHERTYPE_IP);
    ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_p = IPPROTO_UDP;
    ip->ip_off = htons(1); /* 非首片分片（偏移 8 字节） */
    init_header(&header, sizeof packet);

    assert(parse_packet(&header, packet, &info) == -1);
    printf("PASS: non-first IPv4 fragment rejected\n");
}

int main(void)
{
    test_short_ethernet();
    test_non_ipv4();
    test_short_ip_header();
    test_invalid_ip_header_length();
    test_truncated_ip_header();
    test_short_udp_header();
    test_short_tcp_header();
    test_unsupported_protocol();
    test_valid_udp();
    test_valid_tcp_with_ip_options();
    test_ipv4_non_first_fragment();

    printf("All packet tests passed.\n");

    return 0;
}
