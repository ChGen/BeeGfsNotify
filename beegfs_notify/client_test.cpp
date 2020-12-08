#include <cstdlib>
#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <memory>
#include <string>
#include <stdexcept>
#include <zmqpp/zmqpp.hpp>
#include "Common.h"

int main (int argc, char *argv[]) {

//    if (argc != 2) {
//        std::cerr << "Usage ./client_test <path>" << std::endl;
//        std:exit(1);
//    }

    std::string publisherAddress;
    zmqpp::context ctx;
    zmqpp::socket reqSocker(ctx, zmqpp::socket_type::req);
    reqSocker.connect(zmqpp::endpoint_t("tcp://localhost:4322"));
    zmqpp::message req, resp;
    req << REGISTER_COMMAND;
    req << "/dir1"; // argv[1] /localtmp/dump
    reqSocker.send(req);
    reqSocker.receive(resp);
    std::cout << "Response: " << std::endl;
    for (size_t i = 0; i < resp.parts(); ++i) {
        std::cout << resp.get(i) << std::endl;
    }

    if (resp.parts() < 2) {
        std::cerr << "Incorrect server response" << std::endl;
        std::exit(1);
    }
    std::string pathId = resp.get(0);
    publisherAddress = resp.get(1);
    std::cout << "Subscribing to " << publisherAddress << std::endl;

    zmqpp::socket subSocker(ctx, zmqpp::socket_type::sub);
    subSocker.connect(zmqpp::endpoint_t(publisherAddress));
    subSocker.set(zmqpp::socket_option::subscribe, pathId); //filter incomming messages by path id
    while (1) {
        std::cout << "Receiving..." << std::endl;
        subSocker.receive(resp);
        std::cout << "PUB. Response: " << std::endl;
        for (size_t i = 0; i < resp.parts(); ++i) {
            std::cout << resp.get(i) << std::endl;
        }
    }

    std::cout << "Done." << std::endl;
    return 0;
}
