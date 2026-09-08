#include <stdio.h>
#include <string.h>

#include "capture.h"
#include "flow.h"

static void print_usage(const char *program)
{
    printf("Usage: %s <interface> [options]\n", program);
    printf("\n");
    printf("Options:\n");
    printf("  -f, --filter <expr>    BPF filter expression\n");
    printf("  -o, --csv <file>       Export flow statistics to CSV\n");
    printf("  -j, --json <file>      Export flow statistics to JSON\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  sudo %s eth0\n", program);
    printf("  sudo %s eth0 --filter \"udp\"\n", program);
    printf("  sudo %s eth0 --filter \"udp\" --csv result.csv\n", program);
    printf("  sudo %s eth0 --filter \"udp\" --json result.json\n", program);
}

int main(int argc, char **argv)
{
    if (argc < 2)
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

    const char *interface = argv[1];
    const char *filter = NULL;
    const char *csv_file = NULL;
    const char *json_file = NULL;
    int i = 2;

    while (i < argc)
    {
        if (strcmp(argv[i], "-f") == 0 ||
            strcmp(argv[i], "--filter") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(
                    stderr,
                    "Error: %s requires an argument\n",
                    argv[i]
                );

                return 1;
            }

            filter = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "-o") == 0 ||
                 strcmp(argv[i], "--csv") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(
                    stderr,
                    "Error: %s requires a filename\n",
                    argv[i]
                );

                return 1;
            }

            csv_file = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "-j") == 0 ||
                 strcmp(argv[i], "--json") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(
                    stderr,
                    "Error: %s requires a filename\n",
                    argv[i]
                );

                return 1;
            }

            json_file = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "--help") == 0 ||
                 strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(
                stderr,
                "Error: unknown option '%s'\n",
                argv[i]
            );

            print_usage(argv[0]);
            return 1;
        }
       
    }

    printf("Jitter analyzer started\n");

    if (start_capture(interface, filter) != 0)
    {
        fprintf(
            stderr,
            "Failed to start packet capture\n"
        );

        return 1;
    }

    print_all_flow_stats();

    if (csv_file != NULL)
    {
        if (export_flow_stats_csv(csv_file) != 0)
        {
            fprintf(
                stderr,
                "Failed to export flow statistics\n"
            );

            return 1;
        }

        printf(
            "Flow statistics written to %s\n",
            csv_file
        );
    }
    

    if (json_file != NULL)
    {
        if (export_flow_stats_json(json_file) != 0)
        {
            fprintf(
                stderr,
                "Failed to export flow statistics\n"
            );

            return 1;
        }

        printf(
            "Flow statistics written to %s\n",
            json_file
        );
    }

    return 0;
}