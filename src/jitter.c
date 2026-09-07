#include "jitter.h"

/*
 * Interarrival jitter estimation (RFC 3550, section 6.4.1).
 *
 * Jitter is a smoothed estimate of how much the packet arrival
 * timing varies over time:
 *
 *     D(i) = I(i) - I(i-1)             // variation between intervals
 *     J(i) = J(i-1) + (|D(i)| - J(i-1)) / GAIN
 *
 * where I(i) is the inter-arrival interval of packet i and J(0) = 0.
 * The first packet establishes no jitter; the second establishes the
 * first interval; jitter becomes measurable from the third packet on.
 *
 * The gain of 1/16 is the value recommended by RFC 3550. It is a
 * compromise between responsiveness to real changes and immunity to
 * transient noise, and it can be implemented with a single right shift
 * on platforms without floating-point hardware.
 */

#define JITTER_GAIN 16.0

double jitter_update(
        double previous_jitter,
        double variation)
{
    return
        previous_jitter + (variation - previous_jitter) /
        JITTER_GAIN;
}
