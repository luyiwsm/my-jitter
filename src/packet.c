#include<stdio.h>
#include<pcap.h>
#include<netinet/ether.h>
#include<netinet/ip.h>
#include<arpa/inet.h>

#include"packet.h"

void parse_packet(
    const struct pcap_pkthdr *header, 
    const unsigned char *packet
)
{
    const struct ether_header *eth;
    if (header->len < sizeof(struct ether_header)) {
        fprintf(stderr, "Packet too short for Ethernet header\n");
        return;
    }
    eth = (struct ether_header *)packet;
    printf("Length: %u bytes\n", 
        header->len);
    printf("Destination MAC: %s\n",
         ether_ntoa(
            (struct ether_addr *)eth->ether_dhost)
        );
    printf("Source MAC: %s\n",
         ether_ntoa(
            (struct ether_addr *)eth->ether_shost)
        );

    unsigned short ether_type = ntohs(eth->ether_type);
    printf("Ethernet Header\n");
    printf("Ether Type: 0x%04x\n", ether_type);
    /*
    *IPV4 ONLY
    */
   if(ether_type != ETHERTYPE_IP){
        if(ether_type == ETHERTYPE_ARP){
            printf("Protocol: ARP\n");
        return;}
        else{
            printf("Protocol: Other\n");
        }
        return;
    }
    printf("Protocol: IPv4\n");
    /*
    check whether the complete IP header is available in captured packet
    */
    if(header->len < 
    sizeof(struct ether_header) + sizeof(struct ip)){
        fprintf(stderr, "Packet is too short for IP header\n");
        return;
    }
    const struct ip *ip_header;
    ip_header = (const struct ip *)(packet + sizeof(struct ether_header));

    /*
    IPV4 Header LENGTH
    ip_hl is measured in 4 bytes
    */
    unsigned int ip_header_length;
    ip_header_length = ip_header->ip_hl * 4;
    if(ip_header_length < 20 ){
        printf("Invalid IP header length: \n");
        return;
    }
    printf("\nIP Header\n");
    printf("Source IP: %s\n", 
        inet_ntoa(ip_header->ip_src));
    printf("Destination IP: %s\n",
        inet_ntoa(ip_header->ip_dst));
    printf("TTL: %d\n",
        ip_header->ip_ttl);
    printf("Header Length: %d bytes\n",
        ip_header_length);    
    printf("IP Protocol: %d\n",
        ip_header->ip_p);    
}