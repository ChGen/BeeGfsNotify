#pragma once

#include "BeegfsFileEventLog.h"
#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace zmqpp {
class context;
class socket;
enum class socket_option;
}

class DfsNotifier {
    DfsNotifier(const DfsNotifier&) = delete;
    DfsNotifier& operator=(const DfsNotifier&) = delete;
public:
    DfsNotifier(zmqpp::context &ctx, const std::string &eventsSocket);
    void run();

    std::string checkPathWithSubId(const std::string path);
    std::string rmPathWithSubId(const std::string &path);
private:
    void sender();

    size_t _lastId;
    zmqpp::context &_ctx;
    std::string _eventsSocket;
    std::unordered_map<std::string, size_t> _paths;
    std::queue<BeegfsLogPacket> _packages;
    std::condition_variable _msgWaiter;
    std::mutex _msgMutex, _pathMutex;
};
