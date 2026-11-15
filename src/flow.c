#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "flow.h"
#include "jitter.h"

#define MAX_FLOWS 1024
#define FLOW_TIMEOUT 30.0

static flow_t flows[MAX_FLOWS];
static int flow_count;

/* 按方向性五元组查找；反方向的数据包属于另一条流。 */
static flow_t *find_flow(uint32_t src_ip, uint32_t dst_ip,
                         uint16_t src_port, uint16_t dst_port,
                         uint8_t protocol)
{
    for (int i = 0; i < flow_count; i++) {
        flow_t *flow = &flows[i];
        if (flow->active && flow->src_ip == src_ip &&
            flow->dst_ip == dst_ip && flow->src_port == src_port &&
            flow->dst_port == dst_port && flow->protocol == protocol) {
            return flow;
        }
    }
    return NULL;
}

const flow_t *flow_find(uint32_t src_ip, uint32_t dst_ip,
                        uint16_t src_port, uint16_t dst_port,
                        uint8_t protocol)
{
    return find_flow(src_ip, dst_ip, src_port, dst_port, protocol);
}

/* 每次处理新包时，标记超过 30 秒没有数据的流。 */
static void expire_flows(double now)
{
    for (int i = 0; i < flow_count; i++) {
        double idle = now - flows[i].last_arrival_time;
        if (flows[i].active && idle > FLOW_TIMEOUT) {
            flows[i].active = 0;
            printf("Flow expired (idle %.1f s)\n", idle);
        }
    }
}

static flow_t *create_flow(uint32_t src_ip, uint32_t dst_ip,
                           uint16_t src_port, uint16_t dst_port,
                           uint8_t protocol, double arrival_time)
{
    /* 优先复用失效槽位；没有可复用的槽位时才扩展已用范围。 */
    int slot = 0;
    while (slot < flow_count && flows[slot].active) {
        slot++;
    }
    if (slot == MAX_FLOWS) {
        fprintf(stderr, "Flow table full\n");
        return NULL;
    }
    if (slot == flow_count) {
        flow_count++;
    }

    flow_t *flow = &flows[slot];
    memset(flow, 0, sizeof *flow);
    flow->src_ip = src_ip;
    flow->dst_ip = dst_ip;
    flow->src_port = src_port;
    flow->dst_port = dst_port;
    flow->protocol = protocol;
    flow->last_arrival_time = arrival_time;
    flow->packet_count = 1;
    flow->active = 1;

    printf("Allocating new flow slot %d\n", slot);
    printf("New flow created\n");
    return flow;
}

static void update_flow(flow_t *flow, double arrival_time)
{
    double interval = arrival_time - flow->last_arrival_time;
    if (interval < 0) {
        fprintf(stderr, "Warning: negative packet interval\n");
        return;
    }

    flow->packet_count++;
    flow->total_interval += interval;
    if (interval > flow->max_interval) {
        flow->max_interval = interval;
    }

    /* 第二个包才产生第一个间隔；第三个包起才能比较相邻间隔。 */
    if (flow->packet_count > 2) {
        double variation = interval - flow->last_interval;
        if (variation < 0) {
            variation = -variation;
        }
        flow->total_variation += variation;
        flow->jitter = jitter_update(flow->jitter, variation);
        if (flow->jitter > flow->max_jitter) {
            flow->max_jitter = flow->jitter;
        }
        printf("#%lu  interval=%.3f ms  variation=%.3f ms  jitter=%.3f ms\n",
               flow->packet_count, interval * 1000.0,
               variation * 1000.0, flow->jitter * 1000.0);
    } else {
        printf("#%lu  interval=%.3f ms  jitter=%.3f ms\n",
               flow->packet_count, interval * 1000.0, flow->jitter * 1000.0);
    }

    /* 两个分支共同的状态更新放在一起。 */
    flow->last_interval = interval;
    flow->last_arrival_time = arrival_time;
}

void process_flow(uint32_t src_ip, uint32_t dst_ip,
                  uint16_t src_port, uint16_t dst_port,
                  uint8_t protocol, double arrival_time)
{
    expire_flows(arrival_time);
    flow_t *flow = find_flow(src_ip, dst_ip, src_port, dst_port, protocol);
    if (flow != NULL) {
        update_flow(flow, arrival_time);
    } else if (create_flow(src_ip, dst_ip, src_port, dst_port,
                           protocol, arrival_time) == NULL) {
        fprintf(stderr, "Failed to create flow\n");
    }
}

/* 以下是显示和导出部分；阅读主流程时可以暂时跳过。 */
typedef struct {
    double avg_interval;
    double avg_variation;
} flow_stats_t;

static flow_stats_t calculate_flow_stats(const flow_t *flow)
{
    flow_stats_t stats = {0};
    if (flow->packet_count >= 2) {
        stats.avg_interval = flow->total_interval / (flow->packet_count - 1);
    }
    if (flow->packet_count >= 3) {
        stats.avg_variation = flow->total_variation / (flow->packet_count - 2);
    }
    return stats;
}

static const char *protocol_name(uint8_t protocol)
{
    switch (protocol) {
    case IPPROTO_TCP: return "TCP";
    case IPPROTO_UDP: return "UDP";
    default: return "OTHER";
    }
}

static int flow_ip_strings(const flow_t *flow,
                           char src_ip[INET_ADDRSTRLEN],
                           char dst_ip[INET_ADDRSTRLEN])
{
    struct in_addr src = { .s_addr = flow->src_ip };
    struct in_addr dst = { .s_addr = flow->dst_ip };
    if (inet_ntop(AF_INET, &src, src_ip, INET_ADDRSTRLEN) == NULL ||
        inet_ntop(AF_INET, &dst, dst_ip, INET_ADDRSTRLEN) == NULL) {
        return -1;
    }
    return 0;
}

static void print_final_stats(const flow_t *flow)
{
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    if (flow_ip_strings(flow, src_ip, dst_ip) != 0) {
        fprintf(stderr, "Failed to format flow IP address\n");
        return;
    }
    flow_stats_t stats = calculate_flow_stats(flow);
    printf("Flow         : %s:%u -> %s:%u\n"
           "Protocol     : %s\n"
           "Packets      : %lu\n"
           "Avg interval : %.3f ms\n"
           "Max interval : %.3f ms\n"
           "Avg variation: %.3f ms\n"
           "Max EWMA jitter: %.3f ms\n",
           src_ip, flow->src_port, dst_ip, flow->dst_port,
           protocol_name(flow->protocol), flow->packet_count,
           stats.avg_interval * 1000.0, flow->max_interval * 1000.0,
           stats.avg_variation * 1000.0, flow->max_jitter * 1000.0);
}

void print_all_flow_stats(void)
{
    if (flow_count == 0) {
        printf("\nNo flows captured.\n");
        return;
    }
    printf("\nFlow summary\n------------\n");
    for (int i = 0; i < flow_count; i++) {
        if (flows[i].packet_count >= 2) {
            print_final_stats(&flows[i]);
            printf("\n");
        }
    }
}

/* 两种格式共用文件打开、流遍历和统计计算，仅输出格式不同。 */
static int export_flow_stats(const char *filename, int json)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror(json ? "Failed to open JSON file" : "Failed to open CSV file");
        return -1;
    }

    if (json) {
        fprintf(file, "{\n  \"flows\": [\n");
    } else {
        fprintf(file, "src_ip,dst_ip,src_port,dst_port,protocol,"
                      "packets,avg_interval_ms,max_interval_ms,"
                      "avg_variation_ms,max_ewma_jitter_ms\n");
    }

    int first = 1;
    for (int i = 0; i < flow_count; i++) {
        const flow_t *flow = &flows[i];
        if (flow->packet_count < 2) {
            continue;
        }
        char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
        if (flow_ip_strings(flow, src_ip, dst_ip) != 0) {
            fclose(file);
            return -1;
        }
        flow_stats_t stats = calculate_flow_stats(flow);
        if (json) {
            if (!first) {
                fprintf(file, ",\n");
            }
            fprintf(file,
                    "    {\n"
                    "      \"src_ip\": \"%s\",\n"
                    "      \"dst_ip\": \"%s\",\n"
                    "      \"src_port\": %u,\n"
                    "      \"dst_port\": %u,\n"
                    "      \"protocol\": \"%s\",\n"
                    "      \"packets\": %lu,\n"
                    "      \"avg_interval_ms\": %.3f,\n"
                    "      \"max_interval_ms\": %.3f,\n"
                    "      \"avg_variation_ms\": %.3f,\n"
                    "      \"max_ewma_jitter_ms\": %.3f\n"
                    "    }",
                    src_ip, dst_ip, flow->src_port, flow->dst_port,
                    protocol_name(flow->protocol), flow->packet_count,
                    stats.avg_interval * 1000.0, flow->max_interval * 1000.0,
                    stats.avg_variation * 1000.0, flow->max_jitter * 1000.0);
            first = 0;
        } else {
            fprintf(file, "%s,%s,%u,%u,%s,%lu,%.3f,%.3f,%.3f,%.3f\n",
                    src_ip, dst_ip, flow->src_port, flow->dst_port,
                    protocol_name(flow->protocol), flow->packet_count,
                    stats.avg_interval * 1000.0, flow->max_interval * 1000.0,
                    stats.avg_variation * 1000.0, flow->max_jitter * 1000.0);
        }
    }
    if (json) {
        fprintf(file, "\n  ]\n}\n");
    }
    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    return failed ? -1 : 0;
}

int export_flow_stats_csv(const char *filename)
{
    return export_flow_stats(filename, 0);
}

int export_flow_stats_json(const char *filename)
{
    return export_flow_stats(filename, 1);
}
