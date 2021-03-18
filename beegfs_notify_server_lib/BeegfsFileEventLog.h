#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <set>
#include <functional>
#include <time.h>
#include <utime.h>

#define BEEGFS_EVENTLOG_FORMAT_MAJOR 1
#define BEEGFS_EVENTLOG_FORMAT_MINOR 0

enum class BeegfsFileEventType : uint32_t {
  FLUSH = 0,
  TRUNCATE = 1,
  SETATTR = 2,
  CLOSE_WRITE = 3,
  CREATE = 4,
  MKDIR = 5,
  MKNOD = 6,
  SYMLINK = 7,
  RMDIR = 8,
  UNLINK = 9,
  HARDLINK = 10,
  RENAME = 11,
};

struct BeegfsLogPacket {
  uint16_t formatVersionMajor;
  uint16_t formatVersionMinor;
  uint32_t size;
  uint64_t droppedSeq;
  uint64_t missedSeq;
  BeegfsFileEventType type;
  std::string path;
  std::string entryId;
  std::string parentEntryId;
  std::string targetPath;
  std::string targetParentId;
};

std::string to_string(const BeegfsFileEventType &fileEvent);
std::ostream& operator<<(std::ostream& os, const BeegfsLogPacket& p);

class BeegfsFileEventLog {
    BeegfsFileEventLog(const BeegfsFileEventLog&) = delete;
    BeegfsFileEventLog& operator=(const BeegfsFileEventLog&) = delete;
public:
  struct exception : std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  enum class ReadErrorCode {
    Success,
    ReadFailed,
    VersionMismatch,
    InvalidSize,
    IgnoredPath
  };

  using PathFilterFunc = std::function<bool (std::string)>;

  explicit BeegfsFileEventLog(const std::string &socketPath, PathFilterFunc pathFilterFunc = {});

  std::pair<ReadErrorCode, BeegfsLogPacket> read();
  void removeSockets();

  ~BeegfsFileEventLog();

protected:
  int listenFD = -1;
  int serverFD = -1;
  bool onAccept = false;
  bool onReceive = false;

  std::pair<ReadErrorCode, BeegfsLogPacket> read_packet(int fd);
private:

  PathFilterFunc _pathFilterFunc;
};
