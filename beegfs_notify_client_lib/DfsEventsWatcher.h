#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace zmqpp {
class context;
class socket;
enum class socket_option;
}

/**
 * @brief The DfsEventsWatcher class allows to get notifications on DFS file changes.
 * For now it works with beeGFS & ZeroMQ (zmqpp for C++) technologies.
 */
class DfsEventsWatcher {
    DfsEventsWatcher(const DfsEventsWatcher&) = delete;
    DfsEventsWatcher& operator=(const DfsEventsWatcher&) = delete;
public:
    /** File event callback.
   * watchedDir - dir which was previously added to the watch list.
   * changedObject - path of the changed object
   * operation - operation performed. See BeegfsEvents
   */
  using DfsWatchCallback = std::function<void(
      const std::string &watchedDir, const std::string &changedObject, const std::string &operation)>;

  DfsEventsWatcher(zmqpp::context &context);
  ~DfsEventsWatcher();

  /**
   * @brief addPath add DFS path to watch its changes
   * @param dfsPath - full DFS path
   * @return true, if watch is registered on server successufully
   */
  bool addPath(const std::string &dfsPath);

  /**
   * @brief removePath remove DFS path from server watch
   * @param dfsPath - full DFS path, as in addPath()
   * @return true, if watched path is unregistered successfully
   */
  bool removePath(const std::string &dfsPath);

  /**
   * @brief start connect to server & start files watching thread
   * @param dfsHost server host in the format "tcp://localhost:4322"
   * @param callback std::function callback for received DFS events. Called from watching thread
   * @return true, if all operations were successfull
   */
  bool start(const std::string &dfsHost, const DfsEventsWatcher::DfsWatchCallback &callback);
  /**
   * @brief stop disconnect & finish DFS watching thread
   */
  void stop();


private:
  bool processWatch(const std::string &dfsPath,
                    const std::string &cmd,
                    zmqpp::socket_option sockOption);

  zmqpp::context &_context;
  std::unique_ptr<zmqpp::socket> _reqSocket, _subSocket;
  std::thread _watcher;
  std::mutex _socketsMutex;
};
