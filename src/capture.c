#include <stdio.h>
#include <pcap.h>
#include <signal.h>

#include "capture.h"
#include "packet.h"

static pcap_t *capture_handle = NULL;
static void handle_sigint(int signal)
{
    (void)signal;

    if (capture_handle != NULL)
    {
        pcap_breakloop(capture_handle);
    }
}
static void packet_handler(
        unsigned char *args,
        const struct pcap_pkthdr *header,
        const unsigned char *packet)
{
    (void)args;

    /*printf("Packet captured\n");

    printf("Time: %ld.%06ld\n",
           (long)header->ts.tv_sec,
           (long)header->ts.tv_usec);*/

    parse_packet(header, packet);

   // printf("\n");
}



int start_capture(const char *device, const char *filter)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t *handle;


    handle = pcap_open_live(
            device,
            BUFSIZ,
            1,
            1000,
            errbuf
    );


    if(handle == NULL)
    {
        fprintf(stderr,
                "Error: %s\n",
                errbuf);

        return -1;
    }
    capture_handle = handle;
    signal(SIGINT, handle_sigint);


    printf("Listening on %s\n", device);


    if (filter != NULL)
    {
    struct bpf_program fp;

    if (pcap_compile(
            handle,
            &fp,
            filter,
            1,
            PCAP_NETMASK_UNKNOWN) == -1)
    {
        fprintf(stderr,
                "Failed to compile filter: %s\n",
                pcap_geterr(handle));

        pcap_close(handle);
        capture_handle = NULL;

        return -1;
    }

    if (pcap_setfilter(handle, &fp) == -1)
    {
        fprintf(stderr,
                "Failed to set filter: %s\n",
                pcap_geterr(handle));

        pcap_freecode(&fp);
        pcap_close(handle);
        capture_handle = NULL;

        return -1;
    }

    pcap_freecode(&fp);

    printf("Filter: %s\n", filter);
        }

    int ret = pcap_loop(
            handle,
            0,
            packet_handler,
            NULL
    );
    if (ret == -1)
    {
        fprintf(stderr,
            "Error: %s\n",
            pcap_geterr(handle));

        pcap_close(handle);
        capture_handle = NULL;

        return -1;
    }

pcap_close(handle);
capture_handle = NULL;

if (ret == -2)
{
    printf("\nStopping analyzer...\n");
}

return 0;

}