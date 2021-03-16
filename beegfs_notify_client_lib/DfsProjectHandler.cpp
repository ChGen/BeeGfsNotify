#include "DfsProjectHandler.h"
#include "Common.h"
#include <zmqpp/zmqpp.hpp>

DfsProjectHandler::DfsProjectHandler(zmqpp::context &context)
    : _context(context) {}

bool DfsProjectHandler::sendCommandToDfs(const std::string &dfsPath, const std::string &command) {
    _socketMutex.lock();
    if (!_handlerSocket) {
         _socketMutex.unlock();
         return false;
    }
    zmqpp::message req, resp;
    req << command;
    req << dfsPath;
    _handlerSocket->send(req);
    _handlerSocket->receive(resp);
    _socketMutex.unlock();
    if (resp.parts() < 1 || resp.get(0).empty()) {
        return false;
    }
    return true;
}

bool DfsProjectHandler::init(const std::string &dfsHost) {
  _socketMutex.lock();
  if (_handlerSocket) {
      _socketMutex.unlock();
      return true;
  }
  _handlerSocket = std::make_unique<zmqpp::socket>(_context, zmqpp::socket_type::req);
  if (!_handlerSocket) {
      _socketMutex.unlock();
      return false;
  }
  _handlerSocket->connect(zmqpp::endpoint_t(dfsHost));
  _socketMutex.unlock();
  return true;
}

