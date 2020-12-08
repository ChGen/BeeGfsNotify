#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <string>
#include <stdexcept>
#include <map>
#include <set>
#include <zmqpp/zmqpp.hpp>
#include "BeegfsFileEventLog.h"
#include "Common.h"

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

std::ostream& operator<<(std::ostream& os, const BeegfsLogPacket& p)
{
   os << to_string(p.type) << " " << p.path;
   return os;
}

//TODO:
// returns subId (stub) - needs Consistent Hashing.
// strip & uniform path before using!
// NOTE: good perf. only for full match, for prefixes iteration is required
// NOTE: add dirname() check as simple prefix
//
static std::unordered_map<std::string, size_t> _paths;

std::string checkPathWithSubId(const std::string path) {
    static size_t id = 0;
    auto it = _paths.find(path);
    if (it != _paths.end()) {
        return std::to_string(it->second);
    }
    ++id;
    _paths.emplace(path, id);
    return std::to_string(id);
}

std::string rmPathWithSubId(const std::string &path) {
    auto it = _paths.find(path);
    if (it != _paths.end()) {
        auto id = std::to_string(it->second);
        _paths.erase(it);
        return id;
    }
    return {};
}

//std::string getPathBySubId(const std::string subId) {
//    return _paths.at(std::stoi(subId));
//}

////

class FsNotifier {
    zmqpp::context &_ctx;
    std::string _eventsSocket;
public:
    FsNotifier(zmqpp::context &ctx, const std::string &eventsSocket): _ctx(ctx), _eventsSocket(eventsSocket){
    }
    void operator()() {
        zmqpp::socket eventsSocker(_ctx, zmqpp::socket_type::pub);
        eventsSocker.bind(zmqpp::endpoint_t("tcp://*:4321"));
        BeegfsFileEventLog log(_eventsSocket);
        while (true)
        {
           const auto data = log.read();
           zmqpp::message msg;
           std::string path;
           size_t pathId;
           std::unordered_map<std::string, size_t>::iterator it;
           switch (data.first) {
           case BeegfsFileEventLog::ReadErrorCode::Success:
              std::cout << data.second << std::endl;
              path = data.second.path;
              it = _paths.find(path);
              if (it == _paths.end()) { // try to match with its directory path
                  if (path.size() > 1) {
                      if (path.back() == '/') // strip trailing slash
                          path = path.erase(path.size() - 1, 1);
                      size_t fnSlash = path.find_last_of('/');
                      if (fnSlash != std::string::npos) // strip file name
                          path = path.erase(fnSlash);
                  }
                  std::cout << " trying: " << path << std::endl;
                  it = _paths.find(path);
                  pathId = (it == _paths.end())? 0: it->second;
              } else {
                  pathId = it->second;
              }
              if (pathId) {
                  msg << std::to_string(pathId); // for filtering
                  msg << path; // matched subscription path
                  msg << data.second.path; // changed object
                  eventsSocker.send(msg);
              }
              continue;
           case BeegfsFileEventLog::ReadErrorCode::VersionMismatch:
              std::cerr << "Invalid Packet Version" << std::endl;
              continue;
           case BeegfsFileEventLog::ReadErrorCode::InvalidSize:
              std::cerr << "Invalid Packet Size" << std::endl;
              continue;
           case BeegfsFileEventLog::ReadErrorCode::ReadFailed:
              std::cerr << "Read Failed" << std::endl;
              continue;
           }
        }
    }
};

int main (int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Usage ./beegfs_notify <path>" << std::endl;
        std:exit(1);
    }

    // PUB/SUB - notifications; REQ/REP - (un)registering file path

    std::cout << zmqpp::version() << std::endl;
    zmqpp::context ctx;


    zmqpp::socket respSocker(ctx, zmqpp::socket_type::rep);
    respSocker.bind(zmqpp::endpoint_t("tcp://*:4322"));

    FsNotifier fsNotifier(ctx, argv[1]); //e.g. /tmp/meta.beegfslog
    std::thread notifierThread(fsNotifier);

    const std::string publisherAddress = "tcp://localhost:4321";
    while (true) {
        zmqpp::message msg, resp;
        std::string subId;
        respSocker.receive(msg);
        if (msg.parts() != 2) {
            std::cerr << "Incorrect message size: " << msg.parts() << std::endl;
            continue;
        }
        if (msg.get(0) == REGISTER_COMMAND) { //register dir (with files) or file
            std::cout << "Subscription for " << msg.get(1) << std::endl;
            subId = checkPathWithSubId(msg.get(1));
            resp << subId; //for filtering on client
            resp << publisherAddress;
        } else if (msg.get(0) == UNREGISTER_COMMAND) {
            std::cout << "Unsubscribe for " << msg.get(1) << std::endl;
            subId = rmPathWithSubId(msg.get(1));
            resp << subId;
            resp << publisherAddress;
        }
        respSocker.send(resp);
    }

    notifierThread.join();
    
    std::cout << "Done." << std::endl;
    return 0;
}
