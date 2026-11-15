#include <stdio.h>
#include <string.h>

#include "capture.h"
#include "flow.h"

static int option_is(const char *arg, const char *short_name, const char *long_name)
{
    return strcmp(arg, short_name) == 0 || strcmp(arg, long_name) == 0;
}

static void print_usage(const char *program)
{
    printf("Usage: %s <interface> [options]\n\n"
           "Options:\n"
           "  -f, --filter <expr>    BPF filter expression\n"
           "  -o, --csv <file>       Export flow statistics to CSV\n"
           "  -j, --json <file>      Export flow statistics to JSON\n"
           "  -h, --help             Show this help message\n\n"
           "Examples:\n"
           "  sudo %s eth0\n"
           "  sudo %s eth0 --filter \"udp\"\n"
           "  sudo %s eth0 --filter \"udp\" --csv result.csv\n"
           "  sudo %s eth0 --filter \"udp\" --json result.json\n",
           program, program, program, program, program);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    if (option_is(argv[1], "-h", "--help")) {
        print_usage(argv[0]);
        return 0;
    }

    const char *interface = argv[1];
    const char *filter = NULL, *csv_file = NULL, *json_file = NULL;

    /* 每个选项都需要一个值，统一检查，随后分配给对应变量。 */
    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        if (option_is(arg, "-h", "--help")) {
            print_usage(argv[0]);
            return 0;
        }
        int is_filter = option_is(arg, "-f", "--filter");
        int is_csv = option_is(arg, "-o", "--csv");
        int is_json = option_is(arg, "-j", "--json");
        if (!is_filter && !is_csv && !is_json) {
            fprintf(stderr, "Error: unknown option '%s'\n", arg);
            print_usage(argv[0]);
            return 1;
        }
        if (i + 1 >= argc) {
            fprintf(stderr, "Error: %s requires %s\n", arg,
                    is_filter ? "an argument" : "a filename");
            return 1;
        }
        const char *value = argv[++i];
        if (is_filter) {
            filter = value;
        } else if (is_csv) {
            csv_file = value;
        } else {
            json_file = value;
        }
    }

    printf("Jitter analyzer started\n");
    if (start_capture(interface, filter) != 0) {
        fprintf(stderr, "Failed to start packet capture\n");
        return 1;
    }
    print_all_flow_stats();

    if (csv_file != NULL) {
        if (export_flow_stats_csv(csv_file) != 0) {
            fprintf(stderr, "Failed to export flow statistics\n");
            return 1;
        }
        printf("Flow statistics written to %s\n", csv_file);
    }
    if (json_file != NULL) {
        if (export_flow_stats_json(json_file) != 0) {
            fprintf(stderr, "Failed to export flow statistics\n");
            return 1;
        }
        printf("Flow statistics written to %s\n", json_file);
    }
    return 0;
}
