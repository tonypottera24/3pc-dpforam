#!/usr/bin/env python3

import argparse
import subprocess
import datetime
import json
import signal
import sys
import psutil

JST = datetime.timezone(datetime.timedelta(hours=+9), 'JST')

criterias = []


def signal_handler(sig, frame):
    print_all()
    print("========== TERMINATED ==========")
    for proc in psutil.process_iter():
        # check whether the process name matches
        if proc.name() == "proto_test":
            proc.kill()
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)


class BenchmarkRecord():
    def __init__(self, stderr):
        self.criteria = {}
        for log in stderr:
            if log.startswith('{'):
                j = json.loads(log)
                criteria = j["name"]
                self.criteria[criteria] = j
                if criteria not in criterias:
                    criterias.append(criteria)

    def print(self):
        for criteria in criterias:
            log = self.criteria[criteria]
            # print(f'log {log}')
            print(
                f'{criteria}, time = {log["time"]}, ct = {log["ct"]}, bandwidth = {log["bandwidth"]}')


def start_benchmark(proto_test_args):
    out = subprocess.run(proto_test_args, stderr=subprocess.PIPE)
    if out.returncode == 0:
        stderr = out.stderr.decode("utf-8").splitlines()
        return BenchmarkRecord(stderr)
    else:
        print('Error')
        print(out.stderr.decode("utf-8"))
        exit(1)


def print_records(records, x_axis):
    for criteria in criterias:
        print(f"========== {criteria} ==========")
        print(f"{criteria} time")
        for x, record in zip(x_axis, records):
            # convert microsecond (us) to millisecond (ms)
            y = round(record.criteria[criteria]["time"] / 1000, 2)
            print(f"({x}, {y})", end="")
        print()

        print(f"{criteria} ct")
        for x, record in zip(x_axis, records):
            y = record.criteria[criteria]["ct"]
            print(f"({x}, {y})", end="")
        print()

        print(f"{criteria} bandwidth")
        for x, record in zip(x_axis, records):
            # convert B to KB
            y = round(record.criteria[criteria]["bandwidth"] / 1000, 2)
            print(f"({x}, {y})", end="")
        print("\n\n")
        #     print(f"{key} COLLISION")
        #     for i, x in enumerate(x_axis):
        #         y = round(records[i]["ct"] / pow(2, x) * 100, 2)
        #         print(f"({x}, {y})", end="")
        #     print("\n")


def print_all():
    if len(LOG_N) > 1:
        print_records(records, LOG_N)
    elif len(DATA_SIZE) > 1:
        print_records(records, DATA_SIZE)
    elif len(TAU) > 1:
        print_records(records, TAU)
    elif len(PSEUDO_DPF_THRESHOLD) > 1:
        print_records(records, PSEUDO_DPF_THRESHOLD)
    else:
        raise NotImplementedError


parser = argparse.ArgumentParser()
parser.add_argument("--party")
args = parser.parse_args()

PARTY = int(args.party)
PORT = 9000 + PARTY
NEXT_PORT = 9000 + (PARTY + 1) % 3

LOG_N = range(1, 100)

# LOG_N = 20
# TAU = range(1, 20)

# DATA_SIZE = [4]
DATA_SIZE = [256]
# DATA_SIZE = range(4, 129, 4)

TAU = [5]

# PSEUDO_DPF_THRESHOLD = range(LOG_N)
PSEUDO_DPF_THRESHOLD = [5]

records = []

try:
    for logn in LOG_N:
        for data_size in DATA_SIZE:
            for tau in TAU:
                for pdpf in PSEUDO_DPF_THRESHOLD:
                    print("========== Benchmark start ==========")
                    t1 = datetime.datetime.now(JST)
                    print(f'{t1}')
                    print(
                        f"logn {logn}, data_size {data_size}, tau {tau}, pdpf {pdpf}")
                    proto_test_args = [
                        "./bin/test/proto_test",
                        "--party", str(PARTY),
                        "--port", str(PORT),
                        "--next_party_port", str(NEXT_PORT),
                        "--log_n", str(logn),
                        "--data_size", str(data_size),
                        "--tau", str(tau),
                        "--log_pseudo_dpf_threshold", str(pdpf),
                    ]
                    record = start_benchmark(proto_test_args)
                    record.print()
                    records.append(record)
                    t2 = datetime.datetime.now(JST)
                    print(f'{t2}, duration: {t2 - t1}')
                    print("========== Benchmark end ==========\n")
except:
    print('failed')
finally:
    print_all()

print("========== FINISHED ==========")
