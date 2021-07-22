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
    cxxopts::Options options(argv[0], "3PC DPF-ORAM IMPLEMENTATION");
    options.positional_help("[optional args]").show_positional_help();
    options.add_options()("par", "Party:[eddie|debbie|charlie]", cxxopts::value<string>())("eip", "Eddie's ip", cxxopts::value<string>()->default_value("127.0.0.1"))("dip", "Debbie's ip", cxxopts::value<string>()->default_value("127.0.0.1"))("tau", "Tau", cxxopts::value<uint>()->default_value("3"))("logn", "LogN", cxxopts::value<uint>()->default_value("12"))("d_bytes", "d_bytes", cxxopts::value<uint>()->default_value("4"))("thr", "Threads", cxxopts::value<uint>()->default_value("1"))("iter", "Iterations", cxxopts::value<uint>()->default_value("100"));

    auto result = options.parse(argc, argv);
    if (result.count("par") == 0) {
        cout << "No party specified" << endl;
        cout << options.help({"", "Group"}) << endl;
        return 0;
    }
    string party = result["par"].as<string>();
    string eddie_ip = result["eip"].as<string>();
    string debbie_ip = result["dip"].as<string>();
    uint tau = result["tau"].as<uint>();
    uint log_n = result["logn"].as<uint>();
    uint d_bytes = result["d_bytes"].as<uint>();
    uint threads = result["thr"].as<uint>();
    uint iters = result["iter"].as<uint>();

    omp_set_num_threads(threads);
    int port = 8000;

    Connection *cons[2] = {new SimpleSocket(), new SimpleSocket()};
    AutoSeededRandomPool rnd;
    CTR_Mode<AES>::Encryption prgs[2];
    uchar bytes[96];
    for (uint i = 0; i < 96; i++) {
        bytes[i] = i;
    }
    uint offset_DE = 0;
    uint offset_CE = 32;
    uint offset_CD = 64;

    if (party == "eddie") {
        cout << "Establishing connection with debbie... " << flush;
        cons[0]->InitServer(port);
        cout << "done" << endl;

        cout << "Establishing connection with charlie... " << flush;
        cons[1]->InitServer(port + 1);
        cout << "done" << endl;

        prgs[0].SetKeyWithIV(bytes + offset_DE, 16, bytes + offset_DE + 16);
        prgs[1].SetKeyWithIV(bytes + offset_CE, 16, bytes + offset_CE + 16);
    } else if (party == "debbie") {
        cout << "Connecting with eddie... " << flush;
        cons[1]->InitClient(eddie_ip.c_str(), port);
        cout << "done" << endl;

        cout << "Establishing connection with charlie... " << flush;
        cons[0]->InitServer(port + 2);
        cout << "done" << endl;

        prgs[0].SetKeyWithIV(bytes + offset_CD, 16, bytes + offset_CD + 16);
        prgs[1].SetKeyWithIV(bytes + offset_DE, 16, bytes + offset_DE + 16);
    } else if (party == "charlie") {
        cout << "Connecting with eddie... " << flush;
        cons[0]->InitClient(eddie_ip.c_str(), port + 1);
        cout << "done" << endl;

        cout << "Connecting with debbie... " << flush;
        cons[1]->InitClient(debbie_ip.c_str(), port + 2);
        cout << "done" << endl;

        prgs[0].SetKeyWithIV(bytes + offset_CE, 16, bytes + offset_CE + 16);
        prgs[1].SetKeyWithIV(bytes + offset_CD, 16, bytes + offset_CD + 16);
    } else {
        cout << "Incorrect party: " << party << endl;
        delete cons[0];
        delete cons[1];
        return 0;
    }

    Protocol *test_proto = NULL;
    unsigned long init_wc = current_timestamp();
    test_proto = new DPFORAM(party.c_str(), cons, &rnd, prgs, tau, log_n,
                             d_bytes, true);
    init_wc = current_timestamp() - init_wc;
    std::cout << "Init Wallclock(microsec): " << init_wc << std::endl;

    if (test_proto != NULL) {
        test_proto->Sync();
        test_proto->Test(iters);
        test_proto->Sync();
        delete test_proto;
    }

    cout << "Closing connections... " << flush;
    sleep(1);
    cons[0]->Close();
    cons[1]->Close();
    delete cons[0];
    delete cons[1];
    cout << "done" << endl;

    return 0;
}
