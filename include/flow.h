#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>

typedef struct {
    /* 方向性五元组：IP 为网络字节序，端口为主机字节序。 */
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;

    /* 时间和间隔均以秒为单位。 */
    double last_arrival_time;
    double last_interval;
    double jitter;
    double total_interval;
    double max_interval;
    double total_variation;
    double max_jitter;
    unsigned long packet_count;
    int active;
} flow_t;

void process_flow(uint32_t src_ip, uint32_t dst_ip,
                  uint16_t src_port, uint16_t dst_port,
                  uint8_t protocol, double arrival_time);

/* 返回活动流的只读指针，找不到时返回 NULL。
 * 后续处理数据包可能更新或复用槽位，不要把它当作独立快照。 */
const flow_t *flow_find(uint32_t src_ip, uint32_t dst_ip,
                        uint16_t src_port, uint16_t dst_port,
                        uint8_t protocol);

void print_all_flow_stats(void);
int export_flow_stats_csv(const char *filename);
int export_flow_stats_json(const char *filename);

#endif
