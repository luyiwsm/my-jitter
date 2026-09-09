#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "flow.h"

/*
 * Floating-point comparison with a small tolerance,
 * because jitter accumulates through repeated division.
 */
static int close_enough(double actual, double expected)
{
    return fabs(actual - expected) < 1e-9;
}

int main(void)
{
    /*
     * Flow A: four packets at t = 0, 1, 2, 3.015 seconds.
     *
     * intervals = 1.0, 1.0, 1.015
     * total_interval = 3.015
     *
     * jitter after the 4th packet:
     *   variation = |1.015 - 1.0| = 0.015
     *   J = 0 + (0.015 - 0) / 16 = 0.0009375 s
     */
    process_flow(0x0100007f, 0x0200007f, 1000, 2000, 17, 0.000);
    process_flow(0x0100007f, 0x0200007f, 1000, 2000, 17, 1.000);
    process_flow(0x0100007f, 0x0200007f, 1000, 2000, 17, 2.000);
    process_flow(0x0100007f, 0x0200007f, 1000, 2000, 17, 3.015);

    const flow_t *flow_a =
        flow_find(0x0100007f, 0x0200007f, 1000, 2000, 17);

    assert(flow_a != NULL);
    assert(flow_a->packet_count == 4);
    assert(close_enough(flow_a->total_interval, 3.015));
    assert(close_enough(flow_a->jitter, 0.0009375));

    /*
     * 40 seconds later, Flow A is idle longer than
     * FLOW_TIMEOUT (30 s), so it expires.
     *
     * Flow B then reuses Flow A's inactive slot.
     */
    process_flow(0x0300007f, 0x0400007f, 3000, 4000, 17, 43.015);
    process_flow(0x0300007f, 0x0400007f, 3000, 4000, 17, 44.015);

    /* Flow A must be gone after expiry + slot reuse. */
    assert(flow_find(0x0100007f, 0x0200007f, 1000, 2000, 17) == NULL);

    const flow_t *flow_b =
        flow_find(0x0300007f, 0x0400007f, 3000, 4000, 17);

    assert(flow_b != NULL);
    assert(flow_b->packet_count == 2);

    /*
     * Flow C: a packet with a negative interval must be
     * rejected and must not affect the statistics.
     *
     * t = 100, 99 (rejected), 101
     *
     * The valid interval is 101 - 100 = 1.0 s,
     * so packet_count stays at 2.
     */
    process_flow(0x0500007f, 0x0600007f, 5000, 6000, 17, 100.000);
    process_flow(0x0500007f, 0x0600007f, 5000, 6000, 17, 99.000);
    process_flow(0x0500007f, 0x0600007f, 5000, 6000, 17, 101.000);

    const flow_t *flow_c =
        flow_find(0x0500007f, 0x0600007f, 5000, 6000, 17);

    assert(flow_c != NULL);
    assert(flow_c->packet_count == 2);

    printf("All flow tests passed.\n");

    return 0;
}
