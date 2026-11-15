#ifndef CAPTURE_H
#define CAPTURE_H

/* Ctrl+C 停止抓包；成功返回 0，出错返回 -1。filter 可以为 NULL。 */
int start_capture(const char *device, const char *filter);

#endif
