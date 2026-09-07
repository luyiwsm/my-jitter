# my-jitter

A real-time network jitter analyzer written in C, built on [libpcap](https://www.tcpdump.org/).

Jitter — the variation in packet arrival timing — is the key quality metric for real-time media such as VoIP, video conferencing, and game streaming. This tool captures live traffic on a local interface, groups packets into per-flow streams, and reports the interarrival jitter of each flow in real time.

## Pipeline

```
packet capture  ->  protocol parsing  ->  flow tracking  ->  jitter estimation
   (libpcap)       (Eth / IPv4 / TCP)     (5-tuple table)      (RFC 3550 EWMA)
```

- **Capture** — `pcap_open_live()` with an optional BPF filter.
- **Parse** — Ethernet -> IPv4 -> TCP/UDP headers, with bounds checks.
- **Flow table** — keyed by 5-tuple (src/dst IP, src/dst port, protocol), 30 s idle-timeout aging, up to 1024 concurrent flows.
- **Jitter** — RFC 3550 interarrival jitter (exponentially weighted moving average, gain = 1/16).

## Building

Requires `libpcap`:

```bash
sudo apt install libpcap-dev   # Debian / Ubuntu
```

```bash
make          # build ./jitter
make test     # build and run unit tests
make clean    # remove build artifacts
```

## Usage

```bash
sudo ./jitter <interface> [filter]

# examples
sudo ./jitter eth0
sudo ./jitter eth0 "udp"
sudo ./jitter eth0 "udp port 9999"
```

Press `Ctrl-C` to stop and print per-flow statistics.

## Algorithm

Interarrival jitter, per [RFC 3550 §6.4.1](https://www.rfc-editor.org/rfc/rfc3550#section-6.4.1):

```
D(i) = I(i) - I(i-1)                         // variation between intervals
J(i) = J(i-1) + (|D(i)| - J(i-1)) / 16       // smoothed jitter, J(0) = 0
```

where `I(i)` is the interarrival interval of packet `i`. The gain `1/16` is the value recommended by RFC 3550.
