#include <stdio.h>
#include <pcap.h>

#include "capture.h"
#include "packet.h"


static void packet_handler(
        unsigned char *args,
        const struct pcap_pkthdr *header,
        const unsigned char *packet)
{
    (void)args;

    printf("Packet captured\n");

    printf("Time: %ld.%06ld\n",
           (long)header->ts.tv_sec,
           (long)header->ts.tv_usec);

    parse_packet(header, packet);

    printf("\n");
}



int start_capture(char *device)
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


    printf("Listening on %s\n",
            device);


    int ret = pcap_loop(
            handle,
            0,
            packet_handler,
            NULL
    );
    if(ret == -1)
    {
        fprintf(stderr,
                "Error: %s\n",
                pcap_geterr(handle));
        pcap_close(handle);
        return -1;
    }

    pcap_close(handle);


    return (ret == 0) ? 0 : -1;
}