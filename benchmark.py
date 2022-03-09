#!/usr/bin/env python3

import argparse
import subprocess


def start_benchmark(proto_test_arg):
    out = subprocess.run(proto_test_arg, stderr=subprocess.PIPE)

    if out.returncode == 0:
        stderr = out.stderr.decode("utf-8").splitlines()
        bandwidth = int(stderr[-3].split(" ")[-1])
        execution_time = int(stderr[-2].split(" ")[-1])
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


LOG_N = 20
TAU = range(1, 10)
DATA_SIZE = 4
SSOT_THRESHOLD = 0
PSEUDO_DPF_THRESHOLD = 0

bandwidths = []
execution_times = []

for tau in TAU:
    print(f"tau = {tau}")
    proto_test_arg = [
        "./bin/test/proto_test",
        "--party", str(PARTY),
        "--port", str(PORT),
        "--next_party_port", str(NEXT_PORT),
        "--log_n", str(LOG_N),
        "--data_size", str(DATA_SIZE),
        "--tau", str(tau),
        "--log_ssot_threshold", str(SSOT_THRESHOLD),
        "--log_pseudo_dpf_threshold", str(PSEUDO_DPF_THRESHOLD),
    ]
    bandwidth, execution_time = start_benchmark(proto_test_arg)
    bandwidths.append(bandwidth)
    execution_times.append(execution_time)


print("bandwidths")
for i, tau in enumerate(TAU):
    print(f"({tau}, {bandwidths[i]})", end="")
print()

print("execution_times")
for i, tau in enumerate(TAU):
    print(f"({tau}, {execution_times[i]})", end="")
print()
