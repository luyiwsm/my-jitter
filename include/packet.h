#ifndef PACKET_H
#define PACKET_H
    #include <pcap.h>
void parse_packet(
        const struct pcap_pkthdr *header,//libpcap 给我们的原始二进制数据
        const unsigned char *packet);
#endif // PACKET_H
