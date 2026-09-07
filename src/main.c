#include <stdio.h>
#include <string.h>

#include "capture.h"
#include "flow.h"

static void print_usage(const char *program)
{
    printf("Usage: %s <interface> [filter]\n", program);
    printf("\n");
    printf("Arguments:\n");
    printf("  interface    Network interface to capture packets from\n");
    printf("  filter       Optional BPF filter expression\n");
    printf("\n");
    printf("Examples:\n");
    printf("  sudo %s eth0\n", program);
    printf("  sudo %s eth0 \"udp\"\n", program);
    printf("  sudo %s eth0 \"port 1900\"\n", program);
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
    {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0)
    {
        print_usage(argv[0]);
        return 0;
    }

    printf("Jitter analyzer started\n");

    if (start_capture(
            argv[1],
            argc == 3 ? argv[2] : NULL) != 0)
    {
        fprintf(stderr, "Failed to start packet capture\n");
        return 1;
    }

    print_all_flow_stats();

    return 0;
}