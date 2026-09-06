#include "jitter.h"

#define JITTER_GAIN 16.0

double jitter_update(
        double previous_jitter,
        double variation)
{
    return 
        previous_jitter + (variation - previous_jitter)/
        JITTER_GAIN;
}