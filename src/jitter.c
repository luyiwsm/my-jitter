#include "jitter.h"

/* variation 是相邻到达间隔之差的绝对值，单位为秒。
 * 用 1/16 的增益平滑它；这不等同于 RTP 的网络时延抖动测量。 */
double jitter_update(double previous_jitter, double variation)
{
    return previous_jitter + (variation - previous_jitter) / 16.0;
}
