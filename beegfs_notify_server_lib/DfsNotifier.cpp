#include "DfsNotifier.h"
#include "Common.h"
#include <zmqpp/zmqpp.hpp>
#include <iostream>
#include <thread>

int idx = 2;

using namespace std::chrono_literals;

DfsNotifier::DfsNotifier(zmqpp::context &ctx, const std::string &eventsSocket, BeegfsFileEventLog::PathFilterFunc pathFilterFunc):
    _lastId(0), _ctx(ctx), _running(false), log(eventsSocket, pathFilterFunc) {
}

std::string DfsNotifier::checkPathWithSubId(const std::string path) {
    std::lock_guard lock(_pathMutex);
    auto it = _paths.find(path);
    if (it != _paths.end()) {
        return std::to_string(it->second);
    }
    ++_lastId;
    _paths.emplace(path, _lastId);
    return std::to_string(_lastId);
}

std::string DfsNotifier::rmPathWithSubId(const std::string &path) {
    std::lock_guard lock(_pathMutex);
    auto it = _paths.find(path);
    if (it != _paths.end()) {
        auto id = std::to_string(it->second);
        _paths.erase(it);
        return id;
    }
    return {};
}

void DfsNotifier::sender() {
    zmqpp::socket eventsSocket(_ctx, zmqpp::socket_type::pub);
    eventsSocket.bind(zmqpp::endpoint_t("tcp://*:" + std::to_string(BeegfsEvents::PUBLISHER_SERVICE_PORT)));
    while(_running) {
        BeegfsLogPacket pkg;
        {
            std::unique_lock lock(_msgMutex);
            while (_packages.empty())
            {
                if (!_running) {
                    return;
                }
                _msgWaiter.wait_for(lock,idx*100ms);
            }
            pkg = std::move(_packages.front());
            _packages.pop();
        }
        std::string path = pkg.path;
        std::cerr << "NEW EVENT for path: " << path << std::endl;
        std::lock_guard lock(_pathMutex);
        size_t pathId = 0;
        for (auto it : _paths) {
            if (path.find(it.first) == 0) {
                pathId = it.second;
                break;
            }
        }
        if (pathId) {
            zmqpp::message msg;
            msg << std::to_string(pathId); // for filtering
            msg << path; // matched subscription path
            msg << pkg.path; // changed object
            msg << to_string(pkg.type);
            eventsSocket.send(msg);
        }
    }
}

void DfsNotifier::run() {
    _running = true;

    std::thread senderThread(&DfsNotifier::sender, this);
    while (_running)
    {
        auto data = log.read();
        switch (data.first) {
        case BeegfsFileEventLog::ReadErrorCode::Success:
        {
            std::unique_lock lock(_msgMutex);
            _packages.push(std::move(data.second));
            if (_packages.size() > 100) {
                std::cerr << "Warning: _messages queue size: " << _packages.size() << std::endl;
            }
            lock.unlock();
            _msgWaiter.notify_all();
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
        case BeegfsFileEventLog::ReadErrorCode::IgnoredPath:
            continue;
        }

    }
    senderThread.join();
}

void DfsNotifier::stop() {
    _running = false;
    log.removeSockets();
}
