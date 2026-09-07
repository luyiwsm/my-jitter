#include <stdio.h>
#include "flow.h"

int main(void)
{
    /*
     * Flow A
     *
     * Four packets:
     *
     * t0 = 0.000 s
     * t1 = 1.000 s
     * t2 = 2.000 s
     * t3 = 3.015 s
     */
    process_flow(
        0x0100007f,
        0x0200007f,
        1000,
        2000,
        17,
        0.000
    );

    process_flow(
        0x0100007f,
        0x0200007f,
        1000,
        2000,
        17,
        1.000
    );

    process_flow(
        0x0100007f,
        0x0200007f,
        1000,
        2000,
        17,
        2.000
    );

    process_flow(
        0x0100007f,
        0x0200007f,
        1000,
        2000,
        17,
        3.015
    );

    /*
     * 40 seconds later.
     *
     * Flow A should expire.
     */
    process_flow(
        0x0300007f,
        0x0400007f,
        3000,
        4000,
        17,
        43.015
    );

    /*
     * Flow A should still appear
     * in the final statistics.
     */
    print_all_flow_stats();

    return 0;
}