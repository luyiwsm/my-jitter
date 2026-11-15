#ifndef JITTER_H
#define JITTER_H

/* 对相邻到达间隔的变化量作 EWMA 平滑，输入、输出单位均为秒。 */
double jitter_update(double previous_jitter, double variation);

#endif
