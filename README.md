# my-jitter

用 C 和 libpcap 实现的网络数据包到达间隔分析器。

程序从指定网卡抓取 Ethernet / IPv4 / TCP、UDP 数据包，按方向性五元组归类，统计到达间隔、相邻间隔的变化量及其 EWMA 平滑值，并支持导出 CSV、JSON。

项目采用单线程、固定大小流表和普通函数，主要用于练习抓包、协议解析、流状态管理和统计计算。源码阅读可从 [代码阅读说明](docs/code-reading.md) 开始。

## 编译

环境：Linux 或 WSL Ubuntu，GCC、GNU Make、libpcap 开发库。在项目根目录执行：

```bash
# Ubuntu / Debian 安装依赖
sudo apt update
sudo apt install build-essential libpcap-dev

make
```

生成可执行文件 `./jitter`。清理编译产物使用 `make clean`；修改头文件后可用 `make -B` 强制重新编译。

## 运行

先用 `ip link` 查看网卡名称，下面以 `eth0` 为例。抓包通常需要相应权限，示例使用 `sudo`。

```bash
# 查看帮助，不需要抓包权限
./jitter --help

# 抓取数据，分析其中支持的 IPv4 TCP/UDP 包
sudo ./jitter eth0

# 仅抓取 UDP，或进一步限定端口
sudo ./jitter eth0 --filter "udp"
sudo ./jitter eth0 --filter "udp port 9999"

# 停止时同时导出 CSV 和 JSON
sudo ./jitter eth0 --filter "udp" --csv result.csv --json result.json
```

按 **Ctrl+C** 停止抓包，随后输出汇总并执行指定的导出。过滤表达式须通过 `-f` 或 `--filter` 传入；程序不接受直接跟在网卡名后的裸表达式。

| 选项 | 含义 |
|---|---|
| `-f, --filter <expr>` | libpcap 的 BPF 过滤表达式 |
| `-o, --csv <file>` | 导出 CSV，覆盖同名文件 |
| `-j, --json <file>` | 导出 JSON，覆盖同名文件 |
| `-h, --help` | 显示帮助 |

网卡名放在选项前。输出目录需要事先存在；未指定导出选项时，仅输出到终端。

## 流的识别与生命周期

每条流由以下五个字段识别：

```text
源 IP、目的 IP、源端口、目的端口、传输层协议
```

例如，`A:5000 → B:9999 / UDP` 与 `B:9999 → A:5000 / UDP` 是两条独立的流。

- 流表最多保存 1024 个槽位，按五元组线性查找。
- 每处理一个解析成功的数据包，先检查各条流是否空闲超过 30 秒；正好 30 秒时不会失效。没有后台定时清理线程。
- 新流优先复用失效槽位；槽位复用时清零原有统计。
- 流表全满且没有可复用槽位时，新的流无法创建；已有流仍可更新。
- 结束汇总和导出仅包含当前槽位中至少收到两个有效包的流，包括尚未被复用的失效流。旧流一旦被覆盖，就不再保留其统计。

因此，导出文件是结束时流表中保留的统计，不是整次运行期间所有历史流的完整记录。

## 统计指标

设一条流的连续有效包到达时间为 `t(n)`，内部时间单位为秒：

```text
I(n) = t(n) - t(n-1)                  到达间隔
V(n) = |I(n) - I(n-1)|                相邻间隔变化量
J(n) = J(n-1) + (V(n) - J(n-1)) / 16  EWMA 平滑值
```

第一个包初始化流；第二个包产生第一个间隔；第三个包起才更新变化量和 EWMA。时间戳早于该流上次有效到达时间的包会被忽略，零间隔则正常计入。

**这里测量的是到达间隔的变化。**程序没有使用发送端时间戳，结果同时受到发送节奏、网络传输及本机观测的影响，不能直接解释为网络单向时延的抖动，也不等同于 RTP 接收端的抖动指标。

| 汇总字段 | 含义 |
|---|---|
| `Flow` | 源 IP:端口 → 目的 IP:端口 |
| `Protocol` | TCP 或 UDP |
| `Packets` | 该槽位中累计接受的有效包数 |
| `Avg interval` | 间隔总和 / (包数 − 1) |
| `Max interval` | 最大到达间隔 |
| `Avg variation` | 变化量总和 / (包数 − 2)；只有两个包时为 0 |
| `Max EWMA jitter` | 该流累计出现的最大 EWMA 值 |

终端和导出文件中的时间指标均使用毫秒。逐包日志中的 `jitter` 是当前 EWMA 值，最终汇总中的 `Max EWMA jitter` 是其历史最大值。

CSV 表头如下，实际为一行：

```csv
src_ip,dst_ip,src_port,dst_port,protocol,packets,avg_interval_ms,max_interval_ms,avg_variation_ms,max_ewma_jitter_ms
```

JSON 以 `flows` 数组保存相同字段；没有符合导出条件的流时，数组为空。CSV 和 JSON 中的时间指标均保留三位小数。文件打开、写入或关闭失败时返回错误。

## 测试

```bash
make test
make asan
```

| 测试文件 | 覆盖内容 |
|---|---|
| `tests/test_jitter.c` | EWMA 更新及变化量为零时的衰减 |
| `tests/test_flow.c` | 流创建与更新、时间戳倒退、超时边界、槽位复用和统计清零 |
| `tests/test_packet.c` | 协议头截断、无效 IPv4 头长度、非支持协议、正常 UDP、带 IPv4 选项的 TCP 和微秒时间戳 |

`make asan` 会构建 `jitter_asan` 和三个测试程序，并自动运行三个测试。它不会自动启动完整抓包程序；需要时单独运行：

```bash
sudo ./jitter_asan eth0 --filter "udp"
```

测试通过说明这些已执行场景未发现相应问题，不代表所有输入或内存问题都已覆盖。

### 生成测试流量

仓库提供 `tests/udp_sender.py`，需要 Python 3。运行前修改脚本中的 `target_ip` 为实际测试目标地址，并确认抓包网卡可以观察到发往该地址的流量。

```bash
# 终端一：抓取发往测试端口的 UDP 包
sudo ./jitter eth0 --filter "udp dst port 9999"

# 终端二：启动发包脚本
python3 tests/udp_sender.py
```

脚本的 `base_interval` 控制基础发送间隔，`jitter_range` 控制随机等待范围，单位均为秒。可以分别用 `0.0` 和 `0.02` 的随机范围比较统计变化；即使设为 `0.0`，实际到达间隔仍可能受到系统调度等因素影响。

## 源码结构

```text
my-jitter/
├── src/
│   ├── main.c          命令行参数、抓包启动和结束导出
│   ├── capture.c       libpcap 抓包、过滤器和包回调
│   ├── packet.c        协议头长度检查、五元组和时间提取
│   ├── flow.c          流表、超时、统计和输出
│   └── jitter.c        EWMA 公式
├── include/            结构体和函数声明
├── tests/              三组单元测试和 UDP 发包脚本
├── docs/code-reading.md
├── Makefile
└── README.md
```

阅读时先沿着一个包的处理路径走：

```text
main → start_capture → pcap_loop
                            ↓ 收到包时调用
                       packet_handler
                            ↓
                       parse_packet
                            ↓ 解析成功
                       process_flow
                       ├─ 标记超时
                       ├─ 找到已有流 → update_flow → jitter_update
                       └─ 找不到     → create_flow
```

先弄懂 `packet_info_t` 如何保存一个包的解析结果，再看 `flow_t` 如何累计多个包的状态。终端格式化和 CSV/JSON 输出可以放到最后阅读。

## 当前范围

- 解析器按 Ethernet 帧布局读取数据，不自动适配其他链路层类型；应选用提供 Ethernet 帧的抓包接口。
- 支持范围以 IPv4 TCP/UDP 为主，尚未实现 IPv6、VLAN 解析、IPv4 分片重组和完整协议合法性校验。
- 使用固定数组和线性扫描，每个包的流表处理为 `O(N)`，其中 `N` 是已使用过的槽位数量。
- 逐包打印日志，没有完成吞吐量基准测试，不能据此宣称支持高速无丢包抓取。
- 目前没有 pcap 文件读取的命令行入口，也没有网络查询服务或多线程处理。
