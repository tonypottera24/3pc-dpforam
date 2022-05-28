#!/usr/bin/env python3

import argparse
import subprocess


def start_benchmark(proto_test_arg):
    out = subprocess.run(
        proto_test_arg, stderr=subprocess.PIPE)
    result = {}
    if out.returncode == 0:
        stderr = out.stderr.decode("utf-8").splitlines()
        for log in stderr:
            title = log.split(" ")[0]
            if title == "TOTAL":
                result["TOTAL_TIME"] = int(log.split(" ")[1])
                result["TOTAL_BANDWIDTH"] = int(log.split(" ")[3])
            elif title == "KEY_TO_INDEX":
                result["KEY_TO_INDEX_TIME"] = int(log.split(" ")[1])
                result["KEY_TO_INDEX_BANDWIDTH"] = int(log.split(" ")[3])
            elif title == "KEY_TO_INDEX_COLLISION":
                result["KEY_TO_INDEX_COLLISION"] = int(log.split(" ")[1])
        for benchmark_key in BENCHMARK_KEY:
            print(f"{benchmark_key} {result[benchmark_key]}")
        import datetime
        print(
            f"{datetime.timedelta(seconds=result['TOTAL_TIME']/1000*1.5)}")
        return result
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

# LOG_N = range(10, 31)  # binary - binary 4
# LOG_N = range(10, 31)  # binary - ZpDebugData
# LOG_N = range(10, 23)  # ZpDebugData - binary

# LOG_N = range(10, 29)  # binary - binary 32
LOG_N = range(10, 28)  # binary - P256
# LOG_N = range(10, 23)  # P256 - binary
# LOG_N = range(10, 23)  # P256 - P256

# LOG_N = 25
# TAU = range(2, 20)
TAU = 5
# DATA_SIZE = 4
DATA_SIZE = 32
# DATA_SIZE = range(4, 129, 4)
SSOT_THRESHOLD = 0
# PSEUDO_DPF_THRESHOLD = range(LOG_N)
PSEUDO_DPF_THRESHOLD = 5

BENCHMARK_KEY = [
    "TOTAL_TIME",
    "TOTAL_BANDWIDTH",
    "KEY_TO_INDEX_TIME",
    "KEY_TO_INDEX_BANDWIDTH",
    "KEY_TO_INDEX_COLLISION"
]

results = {}
for benchmark_key in BENCHMARK_KEY:
    results[benchmark_key] = []

for logn in LOG_N:
    # for tau in TAU:
    # for pdpf in PSEUDO_DPF_THRESHOLD:
    # for data_size in DATA_SIZE:
    print(f"logn = {logn}")
    # print(f"tau = {tau}")
    # print(f"pdpf = {pdpf}")
    # print(f"data_size = {data_size}")
    proto_test_arg = [
        "./bin/test/proto_test",
        "--party", str(PARTY),
        "--port", str(PORT),
        "--next_party_port", str(NEXT_PORT),
        "--log_n", str(logn),
        # "--log_n", str(LOG_N),
        "--data_size", str(DATA_SIZE),
        # "--data_size", str(data_size),
        "--tau", str(TAU),
        # "--tau", str(tau),
        "--log_ssot_threshold", str(SSOT_THRESHOLD),
        "--log_pseudo_dpf_threshold", str(PSEUDO_DPF_THRESHOLD),
        # "--log_pseudo_dpf_threshold", str(pdpf),
    ]
    result = start_benchmark(proto_test_arg)
    for k, v in result.items():
        results[k].append(v)
    print()


for benchmark_key in BENCHMARK_KEY:
    print(benchmark_key)
    # for i, v in enumerate(TAU):
    for i, v in enumerate(LOG_N):
        # for i, v in enumerate(PSEUDO_DPF_THRESHOLD):
        # for i, v in enumerate(DATA_SIZE):
        if benchmark_key == "KEY_TO_INDEX_COLLISION":
            r = round(results[benchmark_key][i] / pow(2, v) * 100, 2)
        else:
            r = round(results[benchmark_key][i] / 1000, 2)
        print(f"({v}, {r})", end="")
    print("\n")
