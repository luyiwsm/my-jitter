#include <stdio.h>
#include <pcap.h>

#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <arpa/inet.h>

#include "packet.h"


void parse_packet(
        const struct pcap_pkthdr *header,
        const unsigned char *packet)
{
    const struct ether_header *eth;
    unsigned short ether_type;

    if (header->caplen < sizeof(struct ether_header))
    {
        printf("Packet too short for Ethernet header\n");
        return;
    }


    eth = (const struct ether_header *)packet;


    printf("Length: %u bytes\n", header->len);


    printf("Destination MAC: %s\n",
           ether_ntoa(
               (const struct ether_addr *)eth->ether_dhost
           ));


    printf("Source MAC: %s\n",
           ether_ntoa(
               (const struct ether_addr *)eth->ether_shost
           ));


    ether_type = ntohs(eth->ether_type);


    printf("Ethernet Header\n");
    printf("Ether Type: 0x%04x\n", ether_type);


    /*
     * Only continue with IPv4 packets.
     */
    if (ether_type != ETHERTYPE_IP)
    {
        if (ether_type == ETHERTYPE_ARP)
        {
            printf("Protocol: ARP\n");
        }
        else if (ether_type == ETHERTYPE_IPV6)
        {
            printf("Protocol: IPv6\n");
        }
        else
        {
            printf("Protocol: Other\n");
        }

        return;
    }


    printf("Protocol: IPv4\n");


    /*
     * Minimum IPv4 header check.
     */
    if (header->caplen <
        sizeof(struct ether_header) +
        sizeof(struct ip))
    {
        printf("Packet too short for IPv4 header\n");
        return;
    }


    const struct ip *ip_header;

    ip_header = (const struct ip *)
        (packet + sizeof(struct ether_header));


    unsigned int ip_header_length;

    ip_header_length =
        ip_header->ip_hl * 4;


    if (ip_header_length < 20)
    {
        printf("Invalid IPv4 header length\n");
        return;
    }


    if (header->caplen <
        sizeof(struct ether_header) +
        ip_header_length)
    {
        printf("Packet too short for complete IPv4 header\n");
        return;
    }


    printf("\nIP Header\n");


    printf("Source IP: %s\n",
           inet_ntoa(ip_header->ip_src));


    printf("Destination IP: %s\n",
           inet_ntoa(ip_header->ip_dst));


    printf("TTL: %d\n",
           ip_header->ip_ttl);


    printf("Header Length: %u bytes\n",
           ip_header_length);


    printf("IP Protocol: %d\n",
           ip_header->ip_p);


    /*
     * Identify transport layer protocol.
     */
    if (ip_header->ip_p == IPPROTO_TCP)
    {
        printf("Transport Protocol: TCP\n");
    }
    else if (ip_header->ip_p == IPPROTO_UDP)
    {
        printf("Transport Protocol: UDP\n");
    }
    else if (ip_header->ip_p == IPPROTO_ICMP)
    {
        printf("Transport Protocol: ICMP\n");
    }
    else
    {
        printf("Transport Protocol: Other\n");
    }
}