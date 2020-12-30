#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <string>
#include <stdexcept>
#include <map>
#include <set>
#include <thread>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zmqpp/zmqpp.hpp>
#include <netdb.h>
#include <arpa/inet.h>
#include "BeegfsFileEventLog.h"
#include "Common.h"
#include "DfsNotifier.h"

template<typename ...Args>
std::string string_format(const std::string& format, Args... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
    if (size <= 0 )
        throw std::runtime_error( "Incorrect format");
    std::vector<char> buff(size, 0);
    snprintf(buff.data(), size, format.c_str(), args...);
    return std::string(buff.data(), buff.data() + size - 1);
}

int hostname_to_ip(char *hostname, char *ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
    if ((he=gethostbyname(hostname)) == NULL) {
        return 1;
    }
    addr_list = (struct in_addr**)he->h_addr_list;
    for (i=0; addr_list[i] != NULL; i++) {
        strcpy(ip, inet_ntoa(*addr_list[i]));
        return 0;
    }
    return 1;
}

std::string getHostName() {
    char hostname[HOST_NAME_MAX], ip[HOST_NAME_MAX];
    int result = gethostname(hostname, HOST_NAME_MAX);
    if (result != 0) return std::string();
    result = hostname_to_ip(hostname, ip);
    return result == 0? std::string(ip): std::string();
}

std::string getPublisherAddress() {
    return "tcp://" + getHostName() + ":" + std::to_string(BeegfsEvents::PUBLISHER_SERVICE_PORT);
}

int main (int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Usage ./beegfs_notify /tmp/meta.beegfslog" << std::endl;
        return 1;
    }

    // PUB/SUB - notifications; REQ/REP - (un)registering file path
    std::cout << zmqpp::version() << std::endl;
    std::cout << getHostName() << std::endl;

    zmqpp::context ctx;

    DfsNotifier fsNotifier(ctx, argv[1]);
    std::thread notifierThread(&DfsNotifier::run, &fsNotifier);

    zmqpp::socket respSocket(ctx, zmqpp::socket_type::rep);
    respSocket.bind(zmqpp::endpoint_t("tcp://*:" + std::to_string(BeegfsEvents::NOTIFICATION_SERVICE_PORT)));

    const std::string publisherAddress = getPublisherAddress(); // "tcp://localhost:4321";
    while (true) {
        zmqpp::message msg, resp;
        std::string subId;
        try {
            respSocket.receive(msg);
        } catch(const std::exception& ex) {
            std::cerr << "respSocket.receive() exception: " << ex.what();
        }

        std::string type;
        if (msg.parts() > 0) {
            type = msg.get(0);
        }

        if (type == BeegfsEvents::GET_PUBLISHER_COMMAND) {
            std::cout << "Get publisher" << std::endl;
            resp << publisherAddress;
        } else if (type == BeegfsEvents::REGISTER_PATH_COMMAND) { //register dir (with files) or file
            std::cout << "Subscription for " << msg.get(1) << std::endl;
            subId = fsNotifier.checkPathWithSubId(msg.get(1));
            resp << subId; //for filtering on client
            resp << publisherAddress;
        } else if (type == BeegfsEvents::UNREGISTER_PATH_COMMAND) {
            std::cout << "Unsubscribe for " << msg.get(1) << std::endl;
            subId = fsNotifier.rmPathWithSubId(msg.get(1));
            resp << subId;
            resp << publisherAddress;
        }
        try {
            respSocket.send(resp);
        } catch(const std::exception& ex) {
            std::cerr << "respSocket.send() exception: " << ex.what();
        }
    }

    notifierThread.join();
    
    std::cout << "Done." << std::endl;
    return 0;
}
