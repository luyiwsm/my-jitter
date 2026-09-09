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
     *
     * The new Flow B should reuse
     * Flow A's inactive slot.
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
     * Send another packet to Flow B.
     *
     * This makes Flow B eligible for
     * final statistics.
     */
    process_flow(
        0x0300007f,
        0x0400007f,
        3000,
        4000,
        17,
        44.015
    );

    /*
     * Flow B should now have:
     *
     * packet_count = 2
     * interval = 1.000 s
     */
    print_all_flow_stats();


    /*
     * Flow C
     *
     * Test invalid packet timestamp.
     *
     * t0 = 100.000 s
     * t1 =  99.000 s  <-- invalid
     * t2 = 101.000 s
     *
     * The negative interval between t0 and t1
     * should be rejected.
     *
     * The invalid packet should NOT:
     *   1. increase packet_count
     *   2. update last_arrival_time
     *
     * Therefore t2 should still be compared
     * against t0, producing:
     *
     * interval = 1.000 s
     * packet_count = 2
     */
    process_flow(
        0x0500007f,
        0x0600007f,
        5000,
        6000,
        17,
        100.000
    );

    process_flow(
        0x0500007f,
        0x0600007f,
        5000,
        6000,
        17,
        99.000
    );

    process_flow(
        0x0500007f,
        0x0600007f,
        5000,
        6000,
        17,
        101.000
    );

    /*
     * Flow C should appear with:
     *
     * Packets      : 2
     * Avg interval : 1000.000 ms
     */
    print_all_flow_stats();

    return 0;
}