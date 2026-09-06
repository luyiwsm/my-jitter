#include <stdio.h>
#include <pcap.h>

#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include "packet.h"
#include "flow.h"

void parse_packet(
        const struct pcap_pkthdr *header,
        const unsigned char *packet)
{
    /*
     * Ethernet header
     */
    if (header->caplen < sizeof(struct ether_header))
    {
        return;
    }

    const struct ether_header *eth =
        (const struct ether_header *)packet;

    /*
     * Only process IPv4
     */
    if (ntohs(eth->ether_type) != ETHERTYPE_IP)
    {
        return;
    }

    /*
     * IPv4 header
     */
    if (header->caplen <
        sizeof(struct ether_header) + sizeof(struct ip))
    {
        return;
    }

    const struct ip *ip_header =
        (const struct ip *)(packet + sizeof(struct ether_header));

    unsigned int ip_header_length =
        ip_header->ip_hl * 4;

    if (ip_header_length < 20)
    {
        return;
    }

    if (header->caplen <
        sizeof(struct ether_header) + ip_header_length)
    {
        return;
    }

    /*
     * Transport layer starts after IPv4 header
     */
    const unsigned char *transport_start =
        packet
        + sizeof(struct ether_header)
        + ip_header_length;

    uint16_t src_port;
    uint16_t dst_port;

    /*
     * UDP
     */
    if (ip_header->ip_p == IPPROTO_UDP)
    {
        if (header->caplen <
            sizeof(struct ether_header)
            + ip_header_length
            + sizeof(struct udphdr))
        {
            return;
        }

        const struct udphdr *udp_header =
            (const struct udphdr *)transport_start;

        src_port = ntohs(udp_header->uh_sport);
        dst_port = ntohs(udp_header->uh_dport);
    }

    /*
     * TCP
     */
    else if (ip_header->ip_p == IPPROTO_TCP)
    {
        if (header->caplen <
            sizeof(struct ether_header)
            + ip_header_length
            + sizeof(struct tcphdr))
        {
            return;
        }

        const struct tcphdr *tcp_header =
            (const struct tcphdr *)transport_start;

        src_port = ntohs(tcp_header->th_sport);
        dst_port = ntohs(tcp_header->th_dport);
    }

    /*
     * Ignore other transport protocols
     */
    else
    {
        return;
    }

    /*
     * libpcap capture timestamp
     */
    double arrival_time =
        (double)header->ts.tv_sec +
        (double)header->ts.tv_usec / 1000000.0;

    /*
     * Pass the packet to flow manager
     */
    process_flow(
        ip_header->ip_src.s_addr,
        ip_header->ip_dst.s_addr,
        src_port,
        dst_port,
        ip_header->ip_p,
        arrival_time
    );
}