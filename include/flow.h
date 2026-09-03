#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>

// Flow-related declarations go here


typedef struct
{
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    double last_arrival_time;
    double last_interval;
    double jitter;
    unsigned long packet_count;
}flow_t;

void process_flow(
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    uint8_t protocol,
    double arrival_time
);

#endif // FLOW_H
