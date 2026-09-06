import socket
import time
import random

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

target_ip = "172.25.133.130"
target_port = 9999

base_interval = 0.1
jitter_range = 0.0

print("UDP sender started")
print(f"Base interval: {base_interval * 1000:.0f} ms")
print(f"Jitter range: +/- {jitter_range * 1000:.0f} ms")

while True:
    message = b"jitter-test"

    sock.sendto(
        message,
        (target_ip, target_port)
    )

    delay = base_interval + random.uniform(
        -jitter_range,
        jitter_range
    )

    delay = max(0.001, delay)

    time.sleep(delay)
