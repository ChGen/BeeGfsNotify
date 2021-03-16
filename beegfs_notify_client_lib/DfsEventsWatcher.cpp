#include "DfsEventsWatcher.h"
#include "Common.h"
#include <algorithm>
#include <zmqpp/zmqpp.hpp>
#include <iostream>

constexpr long POLL_TIMEOUT = 900;   // ms
constexpr int RECEIVE_TIMEOUT = 1000; // ms

DfsEventsWatcher::DfsEventsWatcher(zmqpp::context &context)
    : _context(context) {}

DfsEventsWatcher::~DfsEventsWatcher() { stop(); }

bool DfsEventsWatcher::processWatch(const std::string &dfsPath,
                                    const std::string &cmd,
                                    zmqpp::socket_option sockOption) {

    if (!_reqSocket || !_subSocket) return false;
    zmqpp::message req, resp;
    req << cmd;
    req << dfsPath;
    std::cout << "ProcessWatcher request cmd=" << cmd << " dfsPath=" << dfsPath << "\n";
    _reqSocket->send(req);
    _reqSocket->receive(resp);
    if (resp.parts() < 2 || resp.get(0).empty()) {
        return false;
    }
    std::string pathId = resp.get(0);
    std::cout << "ProcessWatcher response pathId=" << pathId << "\n";
    std::lock_guard<std::mutex> lock(_socketsMutex);
    _subSocket->set(sockOption, pathId);
    return true;
}

bool DfsEventsWatcher::addPath(const std::string &dfsPath) {
  return processWatch(dfsPath, BeegfsEvents::REGISTER_PATH_COMMAND, zmqpp::socket_option::subscribe);
}

bool DfsEventsWatcher::removePath(const std::string &dfsPath) {
  return processWatch(dfsPath, BeegfsEvents::UNREGISTER_PATH_COMMAND, zmqpp::socket_option::unsubscribe);
}

bool DfsEventsWatcher::start(const std::string &dfsHost, const DfsEventsWatcher::DfsWatchCallback &callback) {
  if (_reqSocket && _subSocket) {
      return true;
  }
  _reqSocket = std::make_unique<zmqpp::socket>(_context, zmqpp::socket_type::req);
  _reqSocket->set(zmqpp::socket_option::receive_timeout, RECEIVE_TIMEOUT);
  _reqSocket->connect(zmqpp::endpoint_t(dfsHost));

  zmqpp::message req, resp;
  req << BeegfsEvents::GET_PUBLISHER_COMMAND;
  req << std::string();
  _reqSocket->send(req);
  _reqSocket->receive(resp);
  if (resp.parts() < 1 || resp.get(0).empty()) {
    return false;
  }
  const std::string publisherAddress = resp.get(0);
  std::cout << "start. publisherAddress=" << publisherAddress << "\n";
  _subSocket = std::make_unique<zmqpp::socket>(_context, zmqpp::socket_type::sub);
  _subSocket->connect(zmqpp::endpoint_t(publisherAddress));
  _watcher = std::thread([this, callback, publisherAddress] {
    zmqpp::poller poller;
    {
      std::unique_lock<std::mutex> lock(_socketsMutex);
      if (_subSocket)
        poller.add(*_subSocket.get());
    }
    bool received = false;
    while (_subSocket) {
      zmqpp::message resp;
      {
        std::lock_guard<std::mutex> lock(_socketsMutex);
        if (_subSocket && poller.poll(POLL_TIMEOUT)) {
            _subSocket->receive(resp, false);
            received = true;
        }
      }
      if (!received) {
          std::this_thread::sleep_for(std::chrono::milliseconds(POLL_TIMEOUT));
      }
      received = false;
      if (callback && resp.parts() > 3) {
        const std::string watchedPath = resp.get(1),
                          changedObject = resp.get(2),
                          operation = resp.get(3);
        callback(watchedPath, changedObject, operation);
      }
    }
  });
  return true;
}

void DfsEventsWatcher::stop() {
  {
    std::unique_lock<std::mutex> lock(_socketsMutex);
    _subSocket.reset();
  }
  if (_watcher.joinable())
    _watcher.join();
}
