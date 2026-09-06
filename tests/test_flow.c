#include <stdio.h>
#include <stdint.h>

#include "flow.h"

int main(void)
{
    /*
     * Flow A
     *
     * First packet: t = 0
     */
    process_flow(
        0x0100007f,   /* 127.0.0.1 */
        0x0200007f,   /* 127.0.0.2 */
        1000,
        2000,
        17,           /* UDP */
        0.0
    );

    /*
     * Second packet of Flow A.
     *
     * This establishes its first interval.
     */
    process_flow(
        0x0100007f,
        0x0200007f,
        1000,
        2000,
        17,
        0.1
    );

    /*
     * 40 seconds later:
     *
     * Flow A has been idle for 39.9 seconds,
     * so it should expire.
     *
     * Flow B uses a different 5-tuple.
     */
    process_flow(
        0x0300007f,   /* 127.0.0.3 */
        0x0400007f,   /* 127.0.0.4 */
        3000,
        4000,
        17,           /* UDP */
        40.0
    );

    printf("Flow aging test completed.\n");

    return 0;
}