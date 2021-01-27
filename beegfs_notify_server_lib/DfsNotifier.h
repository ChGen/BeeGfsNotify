#pragma once

#include "BeegfsFileEventLog.h"
#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>


namespace zmqpp {
class context;
class socket;
enum class socket_option;
}

/**
 * @brief The DfsNotifier class allows to subscribe on DFS path changes and filter them before deliver.
 * DFS changes delivered through ZeroMQ messaging technology.
 */
class DfsNotifier {
    DfsNotifier(const DfsNotifier&) = delete;
    DfsNotifier& operator=(const DfsNotifier&) = delete;
public:

    /**
     * @brief DfsNotifier - initialization
     * @param ctx - zmqpp context object
     * @param eventsSocket - DFS events source, UNIX socket file name
     * @param pathFilterFunc - function object for filtering events before send, optional
     */
    DfsNotifier(zmqpp::context &ctx, const std::string &eventsSocket,
                BeegfsFileEventLog::PathFilterFunc pathFilterFunc = {});

    /**
     * @brief run - start reading and processing events (synchronous)
     */
    void run();

    /**
     * @brief stop reading and processing events. Waits until processing finishes.
     */
    void stop();

    /**
     * @brief check path registration and return subsquent subscriber Id
     * @param path is full DFS path to watch
     * @return - subscriber id, matching given path
     */
    std::string checkPathWithSubId(const std::string path);

    /**
     * @brief rmPathWithSubId
     * @param path
     * @return
     */
    std::string rmPathWithSubId(const std::string &path);
private:
    void sender();

    size_t _lastId;
    zmqpp::context &_ctx;
    std::unordered_map<std::string, size_t> _paths;
    std::queue<BeegfsLogPacket> _packages;
    std::condition_variable _msgWaiter;
    std::mutex _msgMutex, _pathMutex;
    std::atomic<bool> _running;
    BeegfsFileEventLog log;
    bool isNewProject(const std::string &path);
};
