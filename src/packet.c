#include <pcap.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "packet.h"

int parse_packet(const struct pcap_pkthdr *header,
                 const unsigned char *packet, packet_info_t *info)
{
    /* 1. 以太网头：先检查长度，再读取内容，只接受 IPv4。 */
    size_t offset = sizeof(struct ether_header);
    if (header->caplen < offset) {
        return -1;
    }
    const struct ether_header *eth = (const struct ether_header *)packet;
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return -1;
    }

    /* 2. IPv4 头：ip_hl 的单位是 4 字节，不能直接当作字节数。 */
    if (header->caplen < offset + sizeof(struct ip)) {
        return -1;
    }
    const struct ip *ip = (const struct ip *)(packet + offset);
    unsigned int ip_length = ip->ip_hl * 4;
    if (ip->ip_v != 4 || ip_length < 20 || header->caplen < offset + ip_length) {
        return -1;
    }
    if ((ntohs(ip->ip_off) & IP_OFFMASK) != 0) {
        return -1;
    }
    offset += ip_length;

    /* 3. 传输层头：TCP、UDP 分别检查最小长度，再取端口。 */
    uint16_t src_port, dst_port;
    if (ip->ip_p == IPPROTO_UDP) {
        if (header->caplen < offset + sizeof(struct udphdr)) {
            return -1;
        }
        const struct udphdr *udp = (const struct udphdr *)(packet + offset);
        src_port = ntohs(udp->uh_sport);
        dst_port = ntohs(udp->uh_dport);
    } else if (ip->ip_p == IPPROTO_TCP) {
        if (header->caplen < offset + sizeof(struct tcphdr)) {
            return -1;
        }
        const struct tcphdr *tcp = (const struct tcphdr *)(packet + offset);
        src_port = ntohs(tcp->th_sport);
        dst_port = ntohs(tcp->th_dport);
    } else {
        return -1;
    }

    /* IP 保持网络字节序，端口转为主机字节序，时间统一为秒。 */
    info->src_ip = ip->ip_src.s_addr;
    info->dst_ip = ip->ip_dst.s_addr;
    info->src_port = src_port;
    info->dst_port = dst_port;
    info->protocol = ip->ip_p;
    info->arrival_time = (double)header->ts.tv_sec +
                         (double)header->ts.tv_usec / 1000000.0;
    return 0;
}
