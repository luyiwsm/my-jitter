#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "flow.h"

#define MAX_FLOWS 1024
#define JITTER_GAIN 16.0

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

static void print_final_stats(const flow_t *flow)
{
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    struct in_addr src_addr;
    struct in_addr dst_addr;

    src_addr.s_addr = flow->src_ip;
    dst_addr.s_addr = flow->dst_ip;

    inet_ntop(
        AF_INET,
        &src_addr,
        src_ip,
        sizeof(src_ip)
    );

    inet_ntop(
        AF_INET,
        &dst_addr,
        dst_ip,
        sizeof(dst_ip)
    );

    double avg_interval = 0.0;
    double avg_variation = 0.0;
    double avg_jitter = 0.0;

    if (flow->packet_count >= 2)
    {
        avg_interval =
            flow->total_interval /
            (flow->packet_count - 1);
    }

    if (flow->packet_count >= 3)
    {
        avg_variation =
            flow->total_variation /
            (flow->packet_count - 2);

        avg_jitter =
            flow->total_jitter /
            (flow->packet_count - 2);
    }

    printf("Flow         : %s:%u -> %s:%u\n",
           src_ip,
           flow->src_port,
           dst_ip,
           flow->dst_port);

    printf("Protocol     : %s\n",
           flow->protocol == IPPROTO_TCP ? "TCP" :
           flow->protocol == IPPROTO_UDP ? "UDP" :
           "OTHER");

    printf("Packets      : %lu\n",
           flow->packet_count);

    printf("Avg interval : %.3f ms\n",
           avg_interval * 1000.0);

    printf("Max interval : %.3f ms\n",
           flow->max_interval * 1000.0);

    printf("Avg variation: %.3f ms\n",
           avg_variation * 1000.0);

    printf("Avg jitter   : %.3f ms\n",
           avg_jitter * 1000.0);

    printf("Max jitter   : %.3f ms\n",
           flow->max_jitter * 1000.0);
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
        if (!same_flow(
                &flows[i],
                src_ip,
                dst_ip,
                src_port,
                dst_port,
                protocol))
        {
            continue;
        }

        flow_t *current_flow = &flows[i];

        current_flow->packet_count++;

        /*
         * Inter-arrival interval
         */
        double current_interval =
            arrival_time -
            current_flow->last_arrival_time;

        current_flow->total_interval +=
            current_interval;

        if (current_interval >
            current_flow->max_interval)
        {
            current_flow->max_interval =
                current_interval;
        }

        /*
         * The second packet establishes
         * the first inter-arrival interval.
         */
        if (current_flow->packet_count == 2)
        {
            current_flow->last_interval =
                current_interval;

            printf(
                "#%lu  interval=%.3f ms  jitter=%.3f ms\n",
                current_flow->packet_count,
                current_interval * 1000.0,
                current_flow->jitter * 1000.0
            );
        }
        else
        {
            /*
             * Variation between consecutive
             * inter-arrival intervals:
             *
             * V(n) = |I(n) - I(n-1)|
             */
            double variation =
                current_interval -
                current_flow->last_interval;

            if (variation < 0)
            {
                variation = -variation;
            }

            current_flow->total_variation +=
                variation;

            /*
             * EWMA jitter:
             *
             * J(n) = J(n-1)
             *      + (V(n) - J(n-1)) / 16
             */
            current_flow->jitter +=
                (variation -
                 current_flow->jitter) /
                JITTER_GAIN;

            current_flow->total_jitter +=
                current_flow->jitter;

            if (current_flow->jitter >
                current_flow->max_jitter)
            {
                current_flow->max_jitter =
                    current_flow->jitter;
            }

            printf(
                "#%lu  interval=%.3f ms  variation=%.3f ms  jitter=%.3f ms\n",
                current_flow->packet_count,
                current_interval * 1000.0,
                variation * 1000.0,
                current_flow->jitter * 1000.0
            );

            current_flow->last_interval =
                current_interval;
        }

        current_flow->last_arrival_time =
            arrival_time;

        return;
    }

    /*
     * Create a new flow
     */
    if (flow_count >= MAX_FLOWS)
    {
        fprintf(stderr, "Flow table full\n");
        return;
    }

    flow_t *new_flow =
        &flows[flow_count];

    memset(
        new_flow,
        0,
        sizeof(flow_t)
    );

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

void print_all_flow_stats(void)
{
    if (flow_count == 0)
    {
        printf("\nNo flows captured.\n");
        return;
    }

    printf("\n");
    printf("Flow summary\n");
    printf("------------\n");

    for (int i = 0; i < flow_count; i++)
    {
        if (flows[i].packet_count < 2)
        {
            continue;
        }

        print_final_stats(&flows[i]);

        printf("\n");
    }
}

