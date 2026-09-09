#ifndef PACKET_H
#define PACKET_H

#include <pcap.h>
#include <stdint.h>

/*
 * Parsed network packet information.
 *
 * src_ip / dst_ip are in network byte order,
 * identical to struct in_addr.s_addr.
 */
typedef struct
{
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    double arrival_time;
} packet_info_t;

/*
 * Parse an Ethernet frame into packet_info_t.
 *
 * Only IPv4 / TCP / UDP packets are recognized;
 * everything else is rejected.
 *
 * Return:
 *   0  on success (info is filled)
 *  -1  if the packet is malformed or unsupported
 */
int parse_packet(
        const struct pcap_pkthdr *header,
        const unsigned char *packet,
        packet_info_t *info);

#endif
