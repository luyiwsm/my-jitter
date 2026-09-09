# my-jitter

基于 `libpcap` 的实时网络抖动分析器，使用 C 语言实现。

A real-time network jitter analyzer written in C and built on `libpcap`.

---

## 简介 / Introduction

`my-jitter` 是一个面向 IPv4 网络流量的实时抖动分析工具。程序通过 `libpcap` 从指定网络接口捕获数据包，对 Ethernet、IPv4、TCP/UDP 协议头进行解析，并根据五元组对数据包进行流分类，随后统计每条流的到达间隔、间隔变化以及 EWMA 抖动。

Jitter analysis is useful for evaluating the timing stability of packet streams in applications such as VoIP, video conferencing, streaming, and other real-time network services.

The analyzer captures packets from a live network interface using `libpcap`, parses Ethernet/IPv4/TCP/UDP headers, groups packets by a directional 5-tuple, and estimates interarrival jitter for each flow.

---

## Features / 功能

* **Live packet capture** — 基于 `libpcap` 实时抓取网络数据包。
* **Protocol parsing** — 解析 Ethernet、IPv4、TCP 和 UDP 报文头，并进行边界检查。
* **5-tuple flow tracking** — 根据源 IP、目的 IP、源端口、目的端口和协议识别网络流。
* **Flow timeout** — 流在 30 秒无数据后进入 inactive 状态。
* **Slot reuse** — inactive flow slot 可以被新的网络流重新利用，避免流表永久占用。
* **Interarrival statistics** — 统计数据包到达间隔、最大间隔和平均间隔。
* **Jitter estimation** — 使用基于 RFC 3550 思路的 EWMA 方法估计到达间隔抖动。
* **CSV / JSON export** — 支持将流统计信息导出为 CSV 和 JSON。
* **Unit tests** — 覆盖 jitter、flow 和 packet parser 三个核心模块。
* **AddressSanitizer** — 使用 AddressSanitizer 对核心测试和完整程序进行内存安全检查。

### English

* Live packet capture with `libpcap`
* Ethernet / IPv4 / TCP / UDP packet parsing
* Directional 5-tuple flow tracking
* 30-second idle flow timeout
* Inactive flow slot reuse
* Interarrival interval statistics
* EWMA-based interarrival jitter estimation inspired by RFC 3550
* CSV and JSON statistics export
* Unit tests for jitter, flow management, and packet parsing
* AddressSanitizer-based memory safety validation

---

## Architecture / 系统架构

```text
                  Network Interface
                         │
                         ▼
                ┌─────────────────┐
                │    libpcap      │
                │ Packet Capture  │
                └────────┬────────┘
                         │
                         ▼
                ┌─────────────────┐
                │  Packet Parser  │
                │ Ethernet / IPv4 │
                │ TCP / UDP       │
                └────────┬────────┘
                         │
                         ▼
                ┌─────────────────┐
                │  Flow Tracking  │
                │   5-tuple       │
                │ timeout / reuse │
                └────────┬────────┘
                         │
                         ▼
                ┌─────────────────┐
                │ Jitter Estimator│
                │      EWMA       │
                └────────┬────────┘
                         │
                 ┌───────┴────────┐
                 ▼                ▼
             Console         CSV / JSON
```

### Module responsibilities / 模块职责

```text
src/capture.c
    Packet capture using libpcap

src/packet.c
    Ethernet / IPv4 / TCP / UDP parsing
    Packet boundary validation

src/flow.c
    5-tuple flow identification
    Flow lifecycle management
    Interarrival statistics
    Flow statistics export

src/jitter.c
    EWMA-based jitter estimation

src/main.c
    Program entry point
    Capture and output control
```

---

## Flow Model / 流模型

每条流使用一个方向性的五元组进行标识：

```text
(src IP,
 dst IP,
 src port,
 dst port,
 protocol)
```

例如：

```text
192.168.1.10:5000
        │
        │ UDP
        ▼
192.168.1.20:9999
```

反方向：

```text
192.168.1.20:9999
        │
        │ UDP
        ▼
192.168.1.10:5000
```

会被视为另一条独立 Flow。

流表最多维护 1024 个 Flow slot。Flow 在超过 30 秒没有收到新的数据包后进入 inactive 状态，其 slot 可以被新的 Flow 重新使用。

---

## Jitter Algorithm / 抖动算法

本项目采用**基于 RFC 3550 思路的到达间隔抖动 EWMA 估计方法**。

对于连续数据包：

```text
I(n) = t(n) - t(n-1)
```

其中 `I(n)` 为当前数据包与前一个数据包之间的到达时间间隔。

相邻两个 interval 的变化量为：

```text
D(n) = I(n) - I(n-1)
```

使用其绝对值计算 variation：

```text
V(n) = |I(n) - I(n-1)|
```

随后使用 EWMA 更新 jitter：

```text
J(n) = J(n-1) + (V(n) - J(n-1)) / 16
```

其中：

* `I(n)`：packet interarrival interval
* `V(n)`：相邻 interval 的绝对变化量
* `J(n)`：EWMA jitter estimate
* `1/16`：平滑增益

该实现用于估计**数据包到达间隔的变化程度**，属于轻量级 interarrival jitter estimator，并非完整的 RTP 接收端实现。

RFC 3550 reference:

* RFC 3550, Section 6.4.1 — RTP Receiver Reports

---

## Build / 编译

### Requirements / 环境要求

* Linux / WSL
* GCC
* GNU Make
* `libpcap`

Debian / Ubuntu：

```bash
sudo apt install build-essential libpcap-dev
```

### Build

```bash
make
```

生成：

```text
./jitter
```

清理：

```bash
make clean
```

---

## Usage / 使用

基本用法：

```bash
sudo ./jitter <interface>
```

例如：

```bash
sudo ./jitter eth0
```

使用 BPF filter：

```bash
sudo ./jitter eth0 "udp"
```

指定 UDP 端口：

```bash
sudo ./jitter eth0 "udp port 9999"
```

停止程序：

```text
Ctrl-C
```

程序退出时输出当前流的统计信息。

---

## Output / 输出

每条 Flow 的统计信息包括：

```text
Flow
Protocol
Packets
Avg interval
Max interval
Avg variation
Max EWMA jitter
```

示例：

```text
Flow         : 127.0.0.1:5000 -> 127.0.0.2:9999
Protocol     : UDP
Packets      : 100
Avg interval : 100.012 ms
Max interval : 108.341 ms
Avg variation: 1.237 ms
Max EWMA jitter: 2.481 ms
```

### CSV

```c
src_ip,dst_ip,src_port,dst_port,protocol,packets,\
avg_interval_ms,max_interval_ms,avg_variation_ms,\
max_ewma_jitter_ms
```

### JSON

统计信息也可以保存为结构化 JSON：

```json
{
  "flows": [
    {
      "src_ip": "127.0.0.1",
      "dst_ip": "127.0.0.2",
      "src_port": 5000,
      "dst_port": 9999,
      "protocol": "UDP",
      "packets": 100,
      "avg_interval_ms": 100.012,
      "max_interval_ms": 108.341,
      "avg_variation_ms": 1.237,
      "max_ewma_jitter_ms": 2.481
    }
  ]
}
```

---

## Testing / 测试

项目包含三个核心测试模块：

```text
tests/test_jitter.c
tests/test_flow.c
tests/test_packet.c
```

运行全部单元测试：

```bash
make test
```

测试覆盖：

### Jitter

* 基本 jitter 更新
* EWMA 更新过程
* 边界情况

### Flow

* Flow 创建
* interarrival interval 计算
* EWMA jitter 更新
* Flow timeout
* inactive slot reuse
* 异常时间戳处理

### Packet parser

* truncated Ethernet frame
* non-IPv4 frame
* truncated IPv4 header
* invalid IPv4 IHL
* truncated UDP header
* truncated TCP header
* unsupported protocol
* valid UDP packet

---

## AddressSanitizer / 内存安全检查

使用 AddressSanitizer 编译并运行测试：

```bash
make asan
```

该目标会分别构建：

```text
jitter_asan
test_jitter_asan
test_flow_asan
test_packet_asan
```

用于检查常见内存安全问题，例如：

* buffer overflow
* use-after-free
* invalid memory access

当前核心测试均已通过 AddressSanitizer 检查。

---

## Project Structure / 项目结构

```text
my-jitter/
├── include/
│   ├── capture.h
│   ├── flow.h
│   ├── jitter.h
│   └── packet.h
│
├── src/
│   ├── capture.c
│   ├── flow.c
│   ├── jitter.c
│   ├── main.c
│   └── packet.c
│
├── tests/
│   ├── test_flow.c
│   ├── test_jitter.c
│   ├── test_packet.c
│   └── udp_sender.py
│
├── Makefile
├── README.md
└── .gitignore
```

---

## Limitations / 当前限制

当前版本有意控制在较小的实现范围内：

* 目前主要支持 Ethernet + IPv4。
* TCP 和 UDP 已支持。
* 当前不支持 IPv6。
* 当前未专门处理 VLAN-tagged Ethernet frames。
* IPv4 fragmentation 不在当前解析范围内。
* Flow 使用固定大小的 1024-slot table。
* Jitter estimator 是基于 interarrival interval 的 EWMA 方法，而不是完整的 RTP/RTCP 实现。
* 当前 Flow tracking 使用线性扫描，Flow 数量较大时查找成本为 `O(N)`。

---

## Future Work / 后续计划

可能的后续改进包括：

* IPv6 packet parsing
* VLAN-aware parsing
* 更高效的 Flow hash table
* 更完善的 Flow history management
* 更丰富的 CLI 参数
* 实时统计汇总
* pcap file offline analysis
* 更完整的性能 benchmark
* RTP-specific jitter analysis

---

## Technical Highlights / 技术亮点

本项目重点练习和实现了以下网络编程与系统开发技术：

```text
C
│
├── libpcap packet capture
├── raw packet parsing
├── IPv4 / TCP / UDP
├── 5-tuple flow tracking
├── timeout-based resource management
├── EWMA statistics
├── CSV / JSON serialization
├── unit testing
├── AddressSanitizer
└── Makefile-based build system
```

项目的核心目标不是复现一个大型网络分析器，而是在较小的代码规模内实现一个完整的：

```text
Packet Capture
      ↓
Packet Parsing
      ↓
Flow Tracking
      ↓
Statistical Analysis
      ↓
Structured Output
```

网络分析处理链。
