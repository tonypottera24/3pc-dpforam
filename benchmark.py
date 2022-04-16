#!/usr/bin/env python3

import argparse
import subprocess


def start_benchmark(proto_test_arg):
    out = subprocess.run(proto_test_arg, stderr=subprocess.PIPE)

    if out.returncode == 0:
        stderr = out.stderr.decode("utf-8").splitlines()
        bandwidth = int(stderr[-2].split(" ")[-1])
        execution_time = int(stderr[-2].split(" ")[1])
        print(f"bandwidth {bandwidth}")
        print(f"execution_time {execution_time}")
        return bandwidth, execution_time
    else:
        print('error')
        print(out.stderr.decode("utf-8"))
        exit(1)


parser = argparse.ArgumentParser()
parser.add_argument("--party")
args = parser.parse_args()

PARTY = int(args.party)
PORT = 9000 + PARTY
NEXT_PORT = 9000 + (PARTY + 1) % 3

LOG_N = range(10, 19)
# LOG_N = 25
# TAU = range(1, 10)
TAU = 5
DATA_SIZE = 4
SSOT_THRESHOLD = 0
# PSEUDO_DPF_THRESHOLD = range(LOG_N)
PSEUDO_DPF_THRESHOLD = 5

bandwidths = []
execution_times = []

for logn in LOG_N:
    print(f"logn = {logn}")
    proto_test_arg = [
        "./bin/test/proto_test",
        "--party", str(PARTY),
        "--port", str(PORT),
        "--next_party_port", str(NEXT_PORT),
        "--log_n", str(logn),
        "--data_size", str(DATA_SIZE),
        "--tau", str(TAU),
        "--log_ssot_threshold", str(SSOT_THRESHOLD),
        "--log_pseudo_dpf_threshold", str(PSEUDO_DPF_THRESHOLD),
    ]
    bandwidth, execution_time = start_benchmark(proto_test_arg)
    bandwidths.append(bandwidth)
    execution_times.append(execution_time)
    print()


print("bandwidths")
for i, v in enumerate(LOG_N):
    print(f"({v}, {round(bandwidths[i] / 1000, 2)})", end="")
print("\n")

print("execution_times")
for i, v in enumerate(LOG_N):
    print(f"({v}, {round(execution_times[i] / 1000, 2)})", end="")
print()
