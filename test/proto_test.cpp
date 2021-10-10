#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <thread>

#include "dpf_oram.h"
#include "simple_socket.h"
#include "util.h"

using namespace CryptoPP;
namespace po = boost::program_options;

void start_server(Connection *conn, const char *ip, const uint port) {
    conn->InitServer(ip, port);
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
        "log_n", po::value<uint64_t>()->default_value(2ULL), "number of data (n = 2^log_n)")(
        "data_size", po::value<uint64_t>()->default_value(4ULL), "data size (bytes)")(
        "tau", po::value<uint64_t>()->default_value(3ULL), "tau, each block include 2^tau data")(
        "ssot_threshold", po::value<uint64_t>()->default_value(1000ULL), "ssot threshold")(
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

    uint64_t log_n = vm["log_n"].as<uint64_t>();
    uint64_t n = 1ULL << log_n;

    uint64_t data_size = vm["data_size"].as<uint64_t>();
    uint64_t tau = vm["tau"].as<uint64_t>();
    uint64_t ssot_threshold = vm["ssot_threshold"].as<uint64_t>();

    uint threads = vm["threads"].as<uint>();
    uint iterations = vm["iterations"].as<uint>();

    omp_set_num_threads(threads);

    Connection *conn[2] = {new SimpleSocket(), new SimpleSocket()};

    fprintf(stderr, "Initilizing server %s:%u...\n", ip.c_str(), port);
    std::thread start_server_thread(start_server, conn[0], ip.c_str(), port);

    fprintf(stderr, "Connecting to %s:%u...\n", next_party_ip.c_str(), next_party_port);
    conn[1]->InitClient(next_party_ip.c_str(), next_party_port);
    fprintf(stderr, "Connecting to %s:%u done.\n", next_party_ip.c_str(), port);

    start_server_thread.join();
    fprintf(stderr, "Initilizing server %s:%u done.\n", ip.c_str(), port);

    fprintf(stderr, "Initilizing PRG...\n");
    AutoSeededRandomPool rnd;
    CTR_Mode<AES>::Encryption prgs[2];
    uchar bytes[96];
    for (uint i = 0; i < 96; i++) {
        bytes[i] = i;
    }
    uint offset[3] = {0, 32, 64};
    // P0 [0:P2]=0 [1:P1]=2
    // P1 [0:P0]=1 [1:P2]=0
    // P2 [0:P1]=2 [1:P0]=1
    prgs[0].SetKeyWithIV(bytes + offset[(party + 2) % 3], 16, bytes + offset[(party + 2) % 3] + 16);
    prgs[1].SetKeyWithIV(bytes + offset[party], 16, bytes + offset[party] + 16);
    fprintf(stderr, "Initilizing PRG done. (%u, %u) (%u, %u)\n", offset[(party + 2) % 3], offset[(party + 2) % 3] + 16, offset[party], offset[party] + 16);

    fprintf(stderr, "Initilizing DPFORAM...\n");
    Protocol *dpf_oram = NULL;
    uint64_t start_time = timestamp();
    dpf_oram = new DPFORAM(party, conn, &rnd, prgs, n, data_size, tau, ssot_threshold);
    uint64_t end_time = timestamp();
    fprintf(stderr, "Initilizing DPFORAM done.\n");
    fprintf(stderr, "Time to initilize DPF ORAM: %llu\n", end_time - start_time);

    if (dpf_oram != NULL) {
        dpf_oram->Test(iterations);
        delete dpf_oram;
    }

    fprintf(stderr, "Closing connections... \n");
    conn[0]->Close();
    conn[1]->Close();
    fprintf(stderr, "Closing connections done.\n");

    return 0;
}
