#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "flow.h"
#include "jitter.h"


#define MAX_FLOWS 1024
#define FLOW_TIMEOUT 30.0 // seconds

static flow_t flows[MAX_FLOWS];
static int flow_count = 0;


/*
 * Check whether a packet belongs to a flow.
 */
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


/*
 * Find an existing flow.
 *
 * Return:
 *   pointer to flow if found
 *   NULL if not found
 */
static flow_t *find_flow(
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol)
{
    for (int i = 0; i < flow_count; i++)
    {
        if (!flows[i].active)
        {
            continue;
        }
        if (same_flow(
                &flows[i],
                src_ip,
                dst_ip,
                src_port,
                dst_port,
                protocol))
        {
            return &flows[i];
        }
    }

    return NULL;
}


static void expire_flows(double current_time)
{
    for (int i = 0; i < flow_count; i++)
    {
        if (!flows[i].active)
        {
            continue;
        }

        double idle_time =
            current_time -
            flows[i].last_arrival_time;

        if (idle_time > FLOW_TIMEOUT)
        {
            flows[i].active = 0;

            printf(
                "Flow expired (idle %.1f s)\n",
                idle_time
            );
        }
    }
}

/*
 * Create a new flow.
 */
static flow_t *create_flow(
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol,
        double arrival_time)
{
    flow_t *flow = NULL;

    /*
     * Reuse an expired (inactive) slot first.
     */
    for (int i = 0; i < flow_count; i++)
    {
        if (!flows[i].active)
        {
            flow = &flows[i];
            break;
        }
    }

    /*
     * Otherwise allocate a new slot at the end.
     */
    if (flow == NULL)
    {
        if (flow_count >= MAX_FLOWS)
        {
            fprintf(stderr, "Flow table full\n");
            return NULL;
        }

        flow = &flows[flow_count];
        flow_count++;
    }

    memset(
        flow,
        0,
        sizeof(flow_t)
    );

    flow->src_ip = src_ip;
    flow->dst_ip = dst_ip;
    flow->src_port = src_port;
    flow->dst_port = dst_port;
    flow->protocol = protocol;

    flow->last_arrival_time = arrival_time;
    flow->packet_count = 1;
    flow->active = 1;

    return flow;
}


/*
 * Update statistics for an existing flow.
 */
static void update_flow(
        flow_t *flow,
        double arrival_time)
{
    flow->packet_count++;

    /*
     * Inter-arrival interval:
     *
     * I(n) = t(n) - t(n-1)
     */
    double current_interval =
        arrival_time -
        flow->last_arrival_time;

    /*
     * Ignore invalid timestamps.
     */
    if (current_interval < 0)
    {
        fprintf(
            stderr,
            "Warning: negative packet interval\n"
        );

        flow->last_arrival_time =
            arrival_time;

        return;
    }

    flow->total_interval +=
        current_interval;

    if (current_interval >
        flow->max_interval)
    {
        flow->max_interval =
            current_interval;
    }

    /*
     * The second packet establishes
     * the first inter-arrival interval.
     */
    if (flow->packet_count == 2)
    {
        flow->last_interval =
            current_interval;

        printf(
            "#%lu  interval=%.3f ms  jitter=%.3f ms\n",
            flow->packet_count,
            current_interval * 1000.0,
            flow->jitter * 1000.0
        );
    }
    else
    {
        /*
         * Variation:
         *
         * V(n) = |I(n) - I(n-1)|
         */
        double variation =
            current_interval -
            flow->last_interval;

        if (variation < 0)
        {
            variation = -variation;
        }

        flow->total_variation +=
            variation;

        /*
         * EWMA jitter:
         *
         * J(n) = J(n-1)
         *      + (V(n) - J(n-1)) / 16
         */
        flow->jitter =
            jitter_update(
                flow->jitter,
                variation
            );
            

        if (flow->jitter >
            flow->max_jitter)
        {
            flow->max_jitter =
                flow->jitter;
        }

        printf(
            "#%lu  interval=%.3f ms  variation=%.3f ms  jitter=%.3f ms\n",
            flow->packet_count,
            current_interval * 1000.0,
            variation * 1000.0,
            flow->jitter * 1000.0
        );

        flow->last_interval =
            current_interval;
    }

    flow->last_arrival_time =
        arrival_time;
}


/*
 * Print final statistics for one flow.
 */
static void print_final_stats(
        const flow_t *flow)
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
    }


    printf(
        "Flow         : %s:%u -> %s:%u\n",
        src_ip,
        flow->src_port,
        dst_ip,
        flow->dst_port
    );

    printf(
        "Protocol     : %s\n",
        flow->protocol == IPPROTO_TCP ? "TCP" :
        flow->protocol == IPPROTO_UDP ? "UDP" :
        "OTHER"
    );

    printf(
        "Packets      : %lu\n",
        flow->packet_count
    );

    printf(
        "Avg interval : %.3f ms\n",
        avg_interval * 1000.0
    );

    printf(
        "Max interval : %.3f ms\n",
        flow->max_interval * 1000.0
    );

    printf(
        "Avg variation: %.3f ms\n",
        avg_variation * 1000.0
    );

    printf(
        "Max EWMA jitter: %.3f ms\n",
        flow->max_jitter * 1000.0
    );
}


/*
 * Process one packet's flow information.
 */
void process_flow(
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol,
        double arrival_time)
{
    expire_flows(arrival_time);

    flow_t *flow = find_flow(
        src_ip,
        dst_ip,
        src_port,
        dst_port,
        protocol
    );

    /*
     * Existing flow.
     */
    if (flow != NULL)
    {
        update_flow(
            flow,
            arrival_time
        );

        return;
    }

    /*
     * New flow.
     */
    flow_t *new_flow = create_flow(
        src_ip,
        dst_ip,
        src_port,
        dst_port,
        protocol,
        arrival_time
    );
    if (new_flow == NULL)
    {
        fprintf(stderr, "Failed to create flow\n");
        return;
    }
}


/*
 * Print statistics for all flows.
 */
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

        print_final_stats(
            &flows[i]
        );

        printf("\n");
    }
}