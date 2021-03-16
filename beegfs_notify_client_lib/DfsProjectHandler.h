#pragma once

#include <string>
#include <memory>
#include <mutex>

namespace zmqpp {
class context;
class socket;
}

/**
 * @brief The DfsProjectHandler class allows to send commands to treat files on the server side.
 * For now it works with beeGFS & ZeroMQ (zmqpp for C++) technologies.
 */
class DfsProjectHandler {
    DfsProjectHandler(const DfsProjectHandler&) = delete;
    DfsProjectHandler& operator=(const DfsProjectHandler&) = delete;
public:

  DfsProjectHandler(zmqpp::context &context);

  /**
   * @brief init connect to server 
   * @param dfsHost server host in the format "tcp://localhost:4322"
   * @return true, if all operations were successfull
   */
  bool init(const std::string &dfsHost);

  /**
   * @brief sendCommandToDfs TODO: rename, out of the lib, - send command to dfs server helper
   * @param dfsPath
   * @param command
   * @return
   */
  bool sendCommandToDfs(const std::string &dfsPath, const std::string &command);

private:
  zmqpp::context &_context;
  std::unique_ptr<zmqpp::socket> _handlerSocket;
  std::mutex _socketMutex;
};
