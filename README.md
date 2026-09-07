# my-jitter

基于 libpcap 的实时网络抖动分析器（C 语言实现）。
A real-time network jitter analyzer written in C, built on [libpcap](https://www.tcpdump.org/).

---

## 简介 / Introduction

抖动（jitter）是数据包到达时间间隔的变化程度，是 VoIP、视频会议、游戏直播等实时媒体最核心的质量指标。本工具在本地网卡上实时抓取流量，将数据包按流（flow）分组，并实时统计每条流的到达间隔抖动。

Jitter — the variation in packet arrival timing — is the key quality metric for real-time media such as VoIP, video conferencing, and game streaming. This tool captures live traffic on a local interface, groups packets into per-flow streams, and reports the interarrival jitter of each flow in real time.

## 工作流程 / Pipeline

```
packet capture  ->  protocol parsing  ->  flow tracking  ->  jitter estimation
   (libpcap)       (Eth / IPv4 / TCP)     (5-tuple table)      (RFC 3550 EWMA)
```

- **抓包 Capture** — 使用 `pcap_open_live()` 实时抓包，支持可选的 BPF 过滤器。
- **解析 Parse** — 逐层解析以太网 → IPv4 → TCP/UDP 报文头，并做边界检查。
- **流表 Flow table** — 以五元组（源/目的 IP、端口、协议）为键，30 秒空闲超时老化，最多 1024 条并发流。
- **抖动 Jitter** — RFC 3550 到达间隔抖动（指数加权移动平均，增益 1/16）。

---

- **Capture** — `pcap_open_live()` with an optional BPF filter.
- **Parse** — Ethernet -> IPv4 -> TCP/UDP headers, with bounds checks.
- **Flow table** — keyed by 5-tuple (src/dst IP, src/dst port, protocol), 30 s idle-timeout aging, up to 1024 concurrent flows.
- **Jitter** — RFC 3550 interarrival jitter (exponentially weighted moving average, gain = 1/16).

## 编译 / Building

需要 `libpcap` / Requires `libpcap`:

```bash
sudo apt install libpcap-dev   # Debian / Ubuntu
```

```bash
make          # 编译 ./jitter        build ./jitter
make test     # 编译并运行单元测试   build and run unit tests
make clean    # 清理编译产物        remove build artifacts
```

## 使用 / Usage

```bash
sudo ./jitter <interface> [filter]

# 示例 / examples
sudo ./jitter eth0
sudo ./jitter eth0 "udp"
sudo ./jitter eth0 "udp port 9999"
```

按 `Ctrl-C` 停止并打印每条流的统计信息。
Press `Ctrl-C` to stop and print per-flow statistics.

## 算法 / Algorithm

到达间隔抖动，依据 [RFC 3550 §6.4.1](https://www.rfc-editor.org/rfc/rfc3550#section-6.4.1)。
Interarrival jitter, per [RFC 3550 §6.4.1](https://www.rfc-editor.org/rfc/rfc3550#section-6.4.1):

```
D(i) = I(i) - I(i-1)                         // 相邻间隔的变化量  variation between intervals
J(i) = J(i-1) + (|D(i)| - J(i-1)) / 16       // 平滑后的抖动，J(0)=0  smoothed jitter, J(0) = 0
```

其中 `I(i)` 是第 `i` 个数据包的到达间隔。增益 `1/16` 是 RFC 3550 推荐的取值。
where `I(i)` is the interarrival interval of packet `i`. The gain `1/16` is the value recommended by RFC 3550.
