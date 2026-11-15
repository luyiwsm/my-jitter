# 简化版代码阅读说明

先看每个函数的输入、输出和职责，再读实现。第一次阅读可以跳过终端显示、CSV 和 JSON 格式化部分。

## 读一个包怎样走完整个流程

```text
main()                         src/main.c
  └─ start_capture()           src/capture.c：打开接口、设置过滤器
       └─ pcap_loop()          libpcap：接收数据包
            └─ packet_handler()          每收到一个包时调用
                 ├─ parse_packet()       src/packet.c：提取五元组和时间
                 └─ process_flow()       src/flow.c：解析成功才执行
                      ├─ expire_flows()  标记超时流
                      ├─ find_flow()     查找同一方向的五元组
                      ├─ 找到：update_flow()
                      │         └─ jitter_update()  src/jitter.c
                      └─ 没找到：create_flow()

Ctrl+C 停止抓包后，main() 显示汇总，再按命令行选项导出文件。
```

`pcap_loop()` 接收的是回调函数的地址。`packet_handler()` 由 libpcap 在收到包时调用，并非 main() 直接逐包调用。

## 先弄懂两个结构体

- `packet_info_t`：解析一个包得到的五元组和到达时间，是本次回调内的局部变量。
- `flow_t`：一条流跨多个包累积的状态，保存在固定大小的全局数组里。

IP 地址保持网络字节序，端口转成主机字节序。内部时间、间隔和抖动统一用秒，输出时乘 1000 转成毫秒。

## 流表处理只做三件事

1. 标记空闲超过 30 秒的流。检查由新包触发，没有后台定时线程。
2. 查找方向性五元组，找到后更新统计。
3. 找不到时，优先复用失效槽位，否则使用新的槽位。最多 1024 个槽位。

第一个包只初始化状态；第二个包产生第一个到达间隔；第三个包起，才有两个间隔可供比较。时间戳倒退的包不更新该流的统计。

统计公式：

```text
interval  = 本次到达时间 - 上次到达时间
variation = |本次 interval - 上次 interval|
jitter    = jitter + (variation - jitter) / 16
```

这里衡量的是到达间隔的变化，不能直接当作使用发送端时间戳的 RTP 网络时延抖动。

## 如何确认修改没破坏功能

在项目目录执行：

```bash
make -B
make test
make asan
```

测试覆盖抖动计算、流更新/超时/槽位复用、协议头截断和正常 UDP/TCP 解析。测试通过仅说明这些已运行场景通过，不代表所有输入都没有问题。

## 本次整理了什么

- `main.c`：统一命令行选项的比较和缺少参数检查。
- `capture.c`：过滤器设置单独成函数，抓包句柄集中关闭。
- `packet.c`：用 `offset` 跟踪当前协议头位置，保留原有长度检查。
- `flow.c`：删除重复查找，集中共同状态更新，合并两种导出的公共流程。
- `jitter.c`：保留一个直接的 EWMA 公式。

接口名称和命令行用法保持不变。仍然是单线程、固定数组、线性查找，没有新增 epoll、线程、动态容器等模块。CSV/JSON 的写入或关闭失败现在会返回错误。

当前解析范围仍以 Ethernet + IPv4 + TCP/UDP 为主；未增加 IPv6、VLAN、IPv4 分片重组或其他链路层支持。
