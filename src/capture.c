#include <stdio.h>
#include <pcap.h>
#include <signal.h>

#include "capture.h"
#include "packet.h"
#include "flow.h"

static pcap_t *capture_handle;

static void handle_sigint(int sig)
{
    (void)sig;
    if (capture_handle != NULL) {
        pcap_breakloop(capture_handle);
    }
}

/* libpcap 每收到一个包就调用这里：解析成功后交给流表处理。 */
static void packet_handler(unsigned char *args,
                           const struct pcap_pkthdr *header,
                           const unsigned char *packet)
{
    (void)args;
    packet_info_t info;
    if (parse_packet(header, packet, &info) == 0) {
        process_flow(info.src_ip, info.dst_ip, info.src_port,
                     info.dst_port, info.protocol, info.arrival_time);
    }
}

static int set_filter(pcap_t *handle, const char *filter)
{
    if (filter == NULL) {
        return 0;
    }
    struct bpf_program program;
    if (pcap_compile(handle, &program, filter, 1, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "Failed to compile filter: %s\n", pcap_geterr(handle));
        return -1;
    }
    int result = pcap_setfilter(handle, &program);
    pcap_freecode(&program);
    if (result == -1) {
        fprintf(stderr, "Failed to set filter: %s\n", pcap_geterr(handle));
        return -1;
    }
    printf("Filter: %s\n", filter);
    return 0;
}

int start_capture(const char *device, const char *filter)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error: %s\n", errbuf);
        return -1;
    }
    capture_handle = handle;
    signal(SIGINT, handle_sigint);
    printf("Listening on %s\n", device);

    int result = set_filter(handle, filter);
    if (result == 0) {
        result = pcap_loop(handle, 0, packet_handler, NULL);
        if (result == -1) {
            fprintf(stderr, "Error: %s\n", pcap_geterr(handle));
        }
    }

    /* 打开成功后，无论正常停止还是出错，都在这里关闭句柄。 */
    pcap_close(handle);
    capture_handle = NULL;
    if (result == PCAP_ERROR_BREAK) {
        printf("\nStopping analyzer...\n");
    }
    return result == -1 ? -1 : 0;
}
