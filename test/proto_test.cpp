#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <thread>

#include "oram.h"
#include "peer.h"
#include "socket.h"
#include "util.h"

namespace po = boost::program_options;

void start_server(Socket *socket, const char *ip, const uint port) {
    socket->InitServer(ip, port);
}

int main(int argc, char *argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()(
        "help", "3PC DPF-ORAM")(
        "party", po::value<uint>(), "party id, 0 | 1 | 2")(
        "ip", po::value<std::string>()->default_value("127.0.0.1"), "server ip")(
        "port", po::value<uint>()->default_value(8080), "server port")(
        "next_party_ip", po::value<std::string>()->default_value("127.0.0.1"), "next party's ip")(
        "next_party_port", po::value<uint>()->default_value(8080), "next party's port")(
        "log_n", po::value<uint>()->default_value(17), "number of data (log)")(
        "data_size", po::value<uint>()->default_value(32), "data size (bytes)")(
        "tau", po::value<uint>()->default_value(5), "tau, number of data included in a block (log)")(
        "log_ssot_threshold", po::value<uint>()->default_value(0), "ssot threshold (log)")(
        "log_pseudo_dpf_threshold", po::value<uint>()->default_value(5), "pseudo dpf threshold (log)")(
        "key_value_rounds", po::value<uint>()->default_value(3), "key-value rounds")(
        "key_value_evalall_threshold", po::value<uint>()->default_value(15), "key-value EvalAll threshold (log)")(
        "threads", po::value<uint>()->default_value(1), "number of threads")(
        "iterations", po::value<uint>()->default_value(10), "number of iterations");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (po::unknown_option const &error) {
        std::cerr << "Invalid option: " << error.get_option_name() << "\n";
        std::cerr << desc << "\n";
        exit(1);
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(1);
    }

    uint party;
    if (vm.count("party")) {
        party = vm["party"].as<uint>();
        assert(0 <= party && party < 3);
    } else {
        printf("Error: party was not set.\n\n");
        std::cout << desc << std::endl;
        exit(1);
    }

    std::string ip = vm["ip"].as<std::string>();
    uint port = vm["port"].as<uint>();

    std::string next_party_ip = vm["next_party_ip"].as<std::string>();
    uint next_party_port = vm["next_party_port"].as<uint>();

    uint log_n = vm["log_n"].as<uint>();
    uint n = 1 << log_n;

    uint data_size = vm["data_size"].as<uint>();
    uint tau = vm["tau"].as<uint>();
    DATA_PER_BLOCK = 1 << tau;
    uint log_ssot_threshold = vm["log_ssot_threshold"].as<uint>();
    SSOT_THRESHOLD = 1 << log_ssot_threshold;

    LOG_PSEUDO_DPF_THRESHOLD = vm["log_pseudo_dpf_threshold"].as<uint>();
    PSEUDO_DPF_THRESHOLD = 1 << LOG_PSEUDO_DPF_THRESHOLD;
    fprintf(stderr, "Pseudo DPF threshold %u\n", PSEUDO_DPF_THRESHOLD);

    KEY_VALUE_ROUNDS = vm["key_value_rounds"].as<uint>();
#ifdef BENCHMARK_KEY_VALUE_HASH
    for (uint round = 0; round < KEY_VALUE_ROUNDS; round++) {
        std::string title = std::string("KEY_VALUE_HASH[") + std::to_string(round) + std::string("]");
        Benchmark::KEY_VALUE_HASH.push_back(Benchmark::Record(title));
    }
#endif

    uint key_value_evalall_threshold = vm["key_value_evalall_threshold"].as<uint>();
    KEY_VALUE_EVALALL_THRESHOLD = 1 << key_value_evalall_threshold;

    // uint threads = vm["threads"].as<uint>();
    uint iterations = vm["iterations"].as<uint>();

    // omp_set_num_threads(threads);

    Peer peer[2];
    // fprintf(stderr, "Initilizing server %s:%u...\n", ip.c_str(), port);
    std::thread start_server_thread(start_server, &peer[0].Socket(), ip.c_str(), port);

    fprintf(stderr, "Connecting to %s:%u...\n", next_party_ip.c_str(), next_party_port);
    peer[1].Socket().InitClient(next_party_ip.c_str(), next_party_port);
    fprintf(stderr, "Connecting to %s:%u done.\n", next_party_ip.c_str(), port);

    start_server_thread.join();
    // fprintf(stderr, "Initilizing server %s:%u done.\n", ip.c_str(), port);

    uchar seed[PRG::SeedSize()];
    RAND_bytes(seed, PRG::SeedSize());
    peer[1].PRG()->SetSeed(seed);
    peer[1].Socket().Write(seed, PRG::SeedSize(), NULL);
    peer[0].Socket().Read(seed, PRG::SeedSize(), NULL);
    peer[0].PRG()->SetSeed(seed);

    ZpBoostData::initAESKey();

    // fprintf(stderr, "Initilizing PRG done. (%u, %u) (%u, %u)\n", offset[(party + 2) % 3], offset[(party + 2) % 3] + 16, offset[party], offset[party] + 16);

    // uint64_t start_time = timestamp();
    // ORAM<BinaryData, BinaryData> oram = ORAM<BinaryData, BinaryData>(party, peer, n, 0, data_size, true);
    // ORAM<BinaryData, ZpBoostData> oram = ORAM<BinaryData, ZpBoostData>(party, peer, n, 0, data_size, true);
    ORAM<ZpBoostData, BinaryData> oram = ORAM<ZpBoostData, BinaryData>(party, peer, n, ZpBoostData().Size(), data_size, true);
    // ORAM<ZpBoostData, ZpBoostData> oram = ORAM<ZpBoostData, ZpBoostData>(party, peer, n, ZpBoostData().Size(), data_size, true);
    // ORAM<ZpData, ZpData> oram = ORAM<ZpData, ZpData>(party, peer, n, ZpData().Size(), data_size, true);

    // uint64_t end_time = timestamp();
    // fprintf(stderr, "Time to initilize DPF ORAM: %llu\n", end_time - start_time);

    oram.Test(iterations);

    for (uint b = 0; b < 2; b++) {
        peer[b].Close();
    }

    return 0;
}
