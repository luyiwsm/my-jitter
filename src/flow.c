#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "flow.h"
#include "jitter.h"


#define MAX_FLOWS 1024
#define FLOW_TIMEOUT 30.0 /* seconds */

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
 * Find an existing active flow.
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


/*
 * Find an active flow by 5-tuple (read-only access).
 *
 * Return:
 *   pointer to the flow if found
 *   NULL if not found
 */
const flow_t *flow_find(
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


/*
 * Expire flows that have been idle for longer than
 * FLOW_TIMEOUT seconds.
 */
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
 *
 * Reuse an inactive slot when possible.
 */
static flow_t *create_flow(
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t src_port,
        uint16_t dst_port,
        uint8_t protocol,
        double arrival_time)
{
    int slot = -1;

    /*
     * First try to reuse an inactive flow slot.
     */
    for (int i = 0; i < flow_count; i++)
    {
        if (!flows[i].active)
        {
            slot = i;
            break;
        }
    }

    /*
     * No inactive slot available.
     * Allocate a new slot if the table is not full.
     */
    if (slot == -1)
    {
        if (flow_count >= MAX_FLOWS)
        {
            fprintf(stderr, "Flow table full\n");
            return NULL;
        }

        slot = flow_count;
        flow_count++;
    }

    flow_t *flow = &flows[slot];

    printf(
        "Allocating new flow slot %d\n",
        slot
    );

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

    printf("New flow created\n");

    return flow;
}


/*
 * Update statistics for an existing flow.
 */
void update_flow(
        flow_t *flow,
        double arrival_time)
{
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

        return;
    }

    flow->packet_count++;

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
 * Calculated statistics derived from a flow.
 */
typedef struct
{
    double avg_interval;
    double avg_variation;
} flow_stats_t;


/*
 * Calculate derived statistics for a flow.
 */
static flow_stats_t calculate_flow_stats(
        const flow_t *flow)
{
    flow_stats_t stats = {0};

    if (flow->packet_count >= 2)
    {
        stats.avg_interval =
            flow->total_interval /
            (flow->packet_count - 1);
    }

    if (flow->packet_count >= 3)
    {
        stats.avg_variation =
            flow->total_variation /
            (flow->packet_count - 2);
    }

    return stats;
}


/*
 * Convert protocol number to a human-readable name.
 */
static const char *protocol_name(uint8_t protocol)
{
    switch (protocol)
    {
        case IPPROTO_TCP:
            return "TCP";

        case IPPROTO_UDP:
            return "UDP";

        default:
            return "OTHER";
    }
}


/*
 * Convert flow IP addresses to strings.
 *
 * Return:
 *   0  on success
 *  -1  on failure
 */
static int flow_ip_strings(
        const flow_t *flow,
        char *src_ip,
        size_t src_ip_size,
        char *dst_ip,
        size_t dst_ip_size)
{
    struct in_addr src_addr;
    struct in_addr dst_addr;

    src_addr.s_addr = flow->src_ip;
    dst_addr.s_addr = flow->dst_ip;

    if (inet_ntop(
            AF_INET,
            &src_addr,
            src_ip,
            src_ip_size) == NULL)
    {
        return -1;
    }

    if (inet_ntop(
            AF_INET,
            &dst_addr,
            dst_ip,
            dst_ip_size) == NULL)
    {
        return -1;
    }

    return 0;
}


/*
 * Print final statistics for one flow.
 */
static void print_final_stats(
        const flow_t *flow)
{
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    if (flow_ip_strings(
            flow,
            src_ip,
            sizeof(src_ip),
            dst_ip,
            sizeof(dst_ip)) != 0)
    {
        fprintf(
            stderr,
            "Failed to format flow IP address\n"
        );

        return;
    }

    flow_stats_t stats =
        calculate_flow_stats(flow);

    printf(
        "Flow         : %s:%u -> %s:%u\n",
        src_ip,
        flow->src_port,
        dst_ip,
        flow->dst_port
    );

    printf(
        "Protocol     : %s\n",
        protocol_name(flow->protocol)
    );

    printf(
        "Packets      : %lu\n",
        flow->packet_count
    );

    printf(
        "Avg interval : %.3f ms\n",
        stats.avg_interval * 1000.0
    );

    printf(
        "Max interval : %.3f ms\n",
        flow->max_interval * 1000.0
    );

    printf(
        "Avg variation: %.3f ms\n",
        stats.avg_variation * 1000.0
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


/*
 * Export statistics for all flows to CSV.
 */
int export_flow_stats_csv(const char *filename)
{
    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {
        perror("Failed to open CSV file");
        return -1;
    }

    fprintf(
        file,
        "src_ip,dst_ip,src_port,dst_port,protocol,"
        "packets,avg_interval_ms,max_interval_ms,"
        "avg_variation_ms,max_ewma_jitter_ms\n"
    );

    for (int i = 0; i < flow_count; i++)
    {
        const flow_t *flow = &flows[i];

        if (flow->packet_count < 2)
        {
            continue;
        }

        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];

        if (flow_ip_strings(
                flow,
                src_ip,
                sizeof(src_ip),
                dst_ip,
                sizeof(dst_ip)) != 0)
        {
            fclose(file);
            return -1;
        }

        flow_stats_t stats =
            calculate_flow_stats(flow);

        fprintf(
            file,
            "%s,%s,%u,%u,%s,%lu,%.3f,%.3f,%.3f,%.3f\n",
            src_ip,
            dst_ip,
            flow->src_port,
            flow->dst_port,
            protocol_name(flow->protocol),
            flow->packet_count,
            stats.avg_interval * 1000.0,
            flow->max_interval * 1000.0,
            stats.avg_variation * 1000.0,
            flow->max_jitter * 1000.0
        );
    }

    fclose(file);

    return 0;
}


/*
 * Export statistics for all flows to JSON.
 */
int export_flow_stats_json(const char *filename)
{
    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {
        perror("Failed to open JSON file");
        return -1;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"flows\": [\n");

    int first_flow = 1;

    for (int i = 0; i < flow_count; i++)
    {
        const flow_t *flow = &flows[i];

        if (flow->packet_count < 2)
        {
            continue;
        }

        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];

        if (flow_ip_strings(
                flow,
                src_ip,
                sizeof(src_ip),
                dst_ip,
                sizeof(dst_ip)) != 0)
        {
            fclose(file);
            return -1;
        }

        flow_stats_t stats =
            calculate_flow_stats(flow);

        if (!first_flow)
        {
            fprintf(file, ",\n");
        }

        fprintf(file, "    {\n");
        fprintf(file, "      \"src_ip\": \"%s\",\n", src_ip);
        fprintf(file, "      \"dst_ip\": \"%s\",\n", dst_ip);
        fprintf(file, "      \"src_port\": %u,\n", flow->src_port);
        fprintf(file, "      \"dst_port\": %u,\n", flow->dst_port);
        fprintf(file, "      \"protocol\": \"%s\",\n",
                protocol_name(flow->protocol));
        fprintf(file, "      \"packets\": %lu,\n", flow->packet_count);
        fprintf(file, "      \"avg_interval_ms\": %.3f,\n",
                stats.avg_interval * 1000.0);
        fprintf(file, "      \"max_interval_ms\": %.3f,\n",
                flow->max_interval * 1000.0);
        fprintf(file, "      \"avg_variation_ms\": %.3f,\n",
                stats.avg_variation * 1000.0);
        fprintf(file, "      \"max_ewma_jitter_ms\": %.3f\n",
                flow->max_jitter * 1000.0);
        fprintf(file, "    }");

        first_flow = 0;
    }

    fprintf(file, "\n");
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");

    fclose(file);

    return 0;
}