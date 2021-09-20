#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <omp.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

#include "cxxopts.hpp"
#include "dpf_oram.h"
#include "simple_socket.h"
#include "util.h"

using namespace CryptoPP;
using namespace std;

int main(int argc, char *argv[]) {
    cxxopts::Options options(argv[0], "3PC DPF-ORAM");
    options.positional_help("[optional args]").show_positional_help();
    options.add_options()(
        "party", "Party:[0|1|2]", cxxopts::value<string>())(
        "next_party_ip", "next party's ip", cxxopts::value<string>()->default_value("127.0.0.1"))(
        "tau", "tau", cxxopts::value<uint>()->default_value("3"))(
        "n", "number of data", cxxopts::value<uint>()->default_value("12"))(
        "data_size", "data_size", cxxopts::value<uint>()->default_value("4"))(
        "thr", "Threads", cxxopts::value<uint>()->default_value("1"))(
        "iter", "Iterations", cxxopts::value<uint>()->default_value("100"));

    auto result = options.parse(argc, argv);
    if (result.count("party") == 0) {
        fprintf(stderr, "No party specified\n");
        fprintf(stderr, "%s\n", options.help({"", "Group"}).c_str());
        return 0;
    }
    uint party = result["party"].as<uint>();
    string next_party_ip = result["next_party_ip"].as<string>();

    uint tau = result["tau"].as<uint>();
    uint n = result["n"].as<uint>();
    uint data_size = result["data_size"].as<uint>();

    uint threads = result["thr"].as<uint>();
    uint iters = result["iter"].as<uint>();

    omp_set_num_threads(threads);
    const uint port = 8000;

    Connection *cons[2] = {new SimpleSocket(), new SimpleSocket()};
    AutoSeededRandomPool rnd;
    CTR_Mode<AES>::Encryption prgs[2];
    uchar bytes[96];
    for (uint i = 0; i < 96; i++) {
        bytes[i] = i;
    }
    uint offset_P2_P1 = 0;
    uint offset_P0_P1 = 32;
    uint offset_P0_P2 = 64;

    fprintf(stderr, "Initilize server on port %u P2...\n", port);
    cons[0]->InitServer(port);
    fprintf(stderr, "Initilize server on port %u P2 done.\n", port);
    fprintf(stderr, "Connecting to %s on port %u...\n", next_party_ip.c_str(), port);
    cons[1]->InitClient(next_party_ip.c_str(), port);
    fprintf(stderr, "Connecting to %s on port %u done.\n", next_party_ip.c_str(), port);

    if (party == 1) {
        // conn 0:P2, 1:P0
        prgs[0].SetKeyWithIV(bytes + offset_P2_P1, 16, bytes + offset_P2_P1 + 16);
        prgs[1].SetKeyWithIV(bytes + offset_P0_P1, 16, bytes + offset_P0_P1 + 16);
    } else if (party == 2) {
        // conn 0:P0, 1:P1
        prgs[0].SetKeyWithIV(bytes + offset_P0_P2, 16, bytes + offset_P0_P2 + 16);
        prgs[1].SetKeyWithIV(bytes + offset_P2_P1, 16, bytes + offset_P2_P1 + 16);
    } else if (party == 0) {
        // conn 0:P1, 1:P2
        prgs[0].SetKeyWithIV(bytes + offset_P0_P1, 16, bytes + offset_P0_P1 + 16);
        prgs[1].SetKeyWithIV(bytes + offset_P0_P2, 16, bytes + offset_P0_P2 + 16);
    } else {
        fprintf(stderr, "Incorrect party: %u\n", party);
        exit(1);
    }

    Protocol *dpf_oram = NULL;
    uint64_t start_time = timestamp();
    dpf_oram = new DPFORAM(party, cons, &rnd, prgs, n, data_size, tau);
    uint64_t end_time = timestamp();
    fprintf(stderr, "Time to initilize DPF ORAM: %" PRIu64 "\n", end_time - start_time);

    if (dpf_oram != NULL) {
        dpf_oram->Test(iters);
        delete dpf_oram;
    }

    fprintf(stderr, "Closing connections... ");
    cons[0]->Close();
    cons[1]->Close();
    fprintf(stderr, "Closing connections done.");

    return 0;
}
