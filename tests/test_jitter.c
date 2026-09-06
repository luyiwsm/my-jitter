#include <stdio.h>
#include <math.h>

#include "jitter.h"

static int test_equal(
        double actual,
        double expected)
{
    double diff = fabs(actual - expected);

    return diff < 1e-9;
}

int main(void)
{
    /*
     * Test 1:
     *
     * previous = 0
     * variation = 16 ms
     *
     * J = 0 + (16 - 0) / 16
     *   = 1 ms
     */
    double result1 =
        jitter_update(0.0, 0.016);

    if (!test_equal(result1, 0.001))
    {
        printf(
            "Test 1 failed: %.9f\n",
            result1
        );

        return 1;
    }

    /*
     * Test 2:
     *
     * previous = 1 ms
     * variation = 17 ms
     *
     * J = 1 + (17 - 1) / 16
     *   = 2 ms
     */
    double result2 =
        jitter_update(0.001, 0.017);

    if (!test_equal(result2, 0.002))
    {
        printf(
            "Test 2 failed: %.9f\n",
            result2
        );

        return 1;
    }

    /*
     * Test 3:
     *
     * No variation should gradually
     * reduce the jitter.
     */
    double result3 =
        jitter_update(0.010, 0.0);

    if (!test_equal(result3, 0.009375))
    {
        printf(
            "Test 3 failed: %.9f\n",
            result3
        );

        return 1;
    }

    printf("All jitter tests passed.\n");

    return 0;
}