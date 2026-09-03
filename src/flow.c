#include <stdio.h>
#include <string.h>

#include "flow.h"

#define MAX_FLOWS 1024

static flow_t flows[MAX_FLOWS];
static int flow_count = 0;


static int same_flow(
        const flow_t *flow,
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol)
{
    return flow->src_ip == src_ip &&
           flow->dst_ip == dst_ip &&
           flow->src_port == src_port &&
           flow->dst_port == dst_port &&
           flow->protocol == protocol;
}


void process_flow(
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol,
        double arrival_time)
{
    for (int i = 0; i < flow_count; i++)
    {
        if (same_flow(
        &flows[i],
        src_ip,
        dst_ip,
        src_port,
        dst_port,
        protocol))
{
    flow_t *current_flow = &flows[i];

    current_flow->packet_count++;


    /*
     * Calculate the interval between
     * this packet and the previous packet.
     */
    double current_interval =
        arrival_time -
        current_flow->last_arrival_time;


    /*
     * The second packet only establishes
     * the first interval.
     *
     * Jitter can be calculated starting
     * from the third packet.
     */
    if (current_flow->packet_count == 2)
    {
        current_flow->last_interval =
            current_interval;

        printf("Existing flow, packet count: %lu\n",
               current_flow->packet_count);

        printf("Packet interval: %.6f seconds\n",
               current_interval);
    }
    else
    {
        /*
         * Inter-arrival jitter:
         *
         * |current_interval - previous_interval|
         */
        double difference =
            current_interval -
            current_flow->last_interval;


        if (difference < 0)
        {
            difference = -difference;
        }


        current_flow->jitter =
    current_flow->jitter +
    (difference - current_flow->jitter) / 16.0;


        printf("Existing flow, packet count: %lu\n",
               current_flow->packet_count);

        printf("Packet interval: %.6f seconds\n",
               current_interval);

        printf("Smooth Jitter: %.6f ms\n",
               current_flow->jitter * 1000.0);


        /*
         * Save current interval for
         * the next packet.
         */
        current_flow->last_interval =
            current_interval;
    }


    /*
     * Save current arrival time.
     */
    current_flow->last_arrival_time =
        arrival_time;


    return;
}
    }


    if (flow_count >= MAX_FLOWS)
    {
        fprintf(stderr, "Flow table full\n");
        return;
    }


    flow_t *new_flow = &flows[flow_count];

    memset(new_flow, 0, sizeof(flow_t));

    new_flow->src_ip = src_ip;
    new_flow->dst_ip = dst_ip;

    new_flow->src_port = src_port;
    new_flow->dst_port = dst_port;

    new_flow->protocol = protocol;

    new_flow->last_arrival_time = arrival_time;

    new_flow->packet_count = 1;


    flow_count++;


    printf("New flow created\n");
}