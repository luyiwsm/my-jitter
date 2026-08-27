#include<stdio.h>
#include<pcap.h>

#include"packet.h"
void parse_packet(
    const struct pcap_pkthdr *header, 
    const unsigned char *packet
)
{
    (void)packet; // Unused parameter
    printf("Length: %u bytes\n\n", header->len);
}