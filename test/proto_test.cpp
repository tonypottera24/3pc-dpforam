#include <cryptopp/aes.h>
#include <cryptopp/cpu.h>
#include <cryptopp/modes.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <thread>

#include "dpf_oram.h"
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
        "log_n", po::value<uint>()->default_value(14), "number of data (log)")(
        "data_size", po::value<uint>()->default_value(8), "data size (bytes)")(
        "tau", po::value<uint>()->default_value(3), "tau, number of data included in a block (log)")(
        "log_ssot_threshold", po::value<uint>()->default_value(20), "ssot threshold (log)")(
        "log_pseudo_dpf_threshold", po::value<uint>()->default_value(4), "pseudo dpf threshold (log)")(
        "threads", po::value<uint>()->default_value(1), "number of threads")(
        "iterations", po::value<uint>()->default_value(100), "number of iterations");

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
    uint log_ssot_threshold = vm["log_ssot_threshold"].as<uint>();
    uint ssot_threshold = 1 << log_ssot_threshold;
    fprintf(stderr, "SSOT threshold %u\n", ssot_threshold);

    uint log_pseudo_dpf_threshold = vm["log_pseudo_dpf_threshold"].as<uint>();
    uint pseudo_dpf_threshold = 1 << log_pseudo_dpf_threshold;
    fprintf(stderr, "Pseudo DPF threshold %u\n", pseudo_dpf_threshold);

    uint threads = vm["threads"].as<uint>();
    uint iterations = vm["iterations"].as<uint>();

    omp_set_num_threads(threads);

    Peer peer[2];
    // fprintf(stderr, "Initilizing server %s:%u...\n", ip.c_str(), port);
    std::thread start_server_thread(start_server, &peer[0].Socket(), ip.c_str(), port);

    fprintf(stderr, "Connecting to %s:%u...\n", next_party_ip.c_str(), next_party_port);
    peer[1].Socket().InitClient(next_party_ip.c_str(), next_party_port);
    fprintf(stderr, "Connecting to %s:%u done.\n", next_party_ip.c_str(), port);

    start_server_thread.join();
    // fprintf(stderr, "Initilizing server %s:%u done.\n", ip.c_str(), port);

    // fprintf(stderr, "HasAESNI %u\n", CryptoPP::HasAESNI());

    uchar bytes[96];
    for (uint i = 0; i < 96; i++) {
        bytes[i] = i;
    }
    uint offset[3] = {0, 32, 64};
    // P0 [0:P2]=0 [1:P1]=2
    // P1 [0:P0]=1 [1:P2]=0
    // P2 [0:P1]=2 [1:P0]=1
    peer[0].PRG().SetKeyWithIV(bytes + offset[(party + 2) % 3], 16, bytes + offset[(party + 2) % 3] + 16);
    peer[1].PRG().SetKeyWithIV(bytes + offset[party], 16, bytes + offset[party] + 16);
    // fprintf(stderr, "Initilizing PRG done. (%u, %u) (%u, %u)\n", offset[(party + 2) % 3], offset[(party + 2) % 3] + 16, offset[party], offset[party] + 16);

    uint64_t start_time = timestamp();
    // DPFORAM<BinaryData, BinaryData> dpf_oram = DPFORAM<BinaryData, BinaryData>(party, peer, n, data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    DPFORAM<BinaryData, ECData> dpf_oram = DPFORAM<BinaryData, ECData>(party, peer, n, data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    // DPFORAM<ECData, BinaryData> dpf_oram = DPFORAM<ECData, BinaryData>(party, peer, n, data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    // DPFORAM<ECData, ECData> dpf_oram = DPFORAM<ECData, ECData>(party, peer, n, data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    uint64_t end_time = timestamp();
    fprintf(stderr, "Time to initilize DPF ORAM: %llu\n", end_time - start_time);

    dpf_oram.Test(iterations);

    for (uint b = 0; b < 2; b++) {
        peer[b].Close();
    }

    return 0;
}
