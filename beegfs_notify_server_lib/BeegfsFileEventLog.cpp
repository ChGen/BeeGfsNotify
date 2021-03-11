#include "BeegfsFileEventLog.h"
#include "Common.h"
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

namespace BeegfsEvents {
    const std::string FLUSH_OPERATION = "Flush";
    const std::string TRUNCATE_OPERATION = "Truncate";
    const std::string SET_ATTR_OPERATION = "SetAttr";
    const std::string CLOSE_AFTER_WRITE_OPERATION = "CloseAfterWrite";
    const std::string CREATE_OPERATION = "Create";
    const std::string MKDIR_OPERATION = "MKdir";
    const std::string MKNOD_OPERATION = "MKnod";
    const std::string SYMLINK_OPERATION = "Symlink";
    const std::string RMDIR_OPERATION = "RMdir";
    const std::string UNLINK_OPERATION = "Unlink";
    const std::string HARDLINK_OPERATION = "Hardlink";
    const std::string RENAME_OPERATION = "Rename";
}

class Reader {
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;
public:
  explicit Reader(const char *data, const ssize_t size)
      : position(data), end(data + size) {}

  template <typename T> T read() { return readRaw<T>(); }

private:
  const char *position;
  const char *const end;

  template <typename T> T readRaw() {
    if (position + sizeof(T) >= end)
      throw std::out_of_range("Read past buffer end");
    const auto x = reinterpret_cast<const T *>(position);
    position += sizeof(T);
    return *x;
  }
};

template <> inline bool Reader::read<bool>() {
  return (0 != readRaw<uint8_t>());
}

template <> inline uint32_t Reader::read<u_int32_t>() {
  return le32toh(readRaw<uint32_t>());
}

template <> inline uint64_t Reader::read<u_int64_t>() {
  return le64toh(readRaw<uint64_t>());
}

template <> inline std::string Reader::read<std::string>() {
  const auto len = read<uint32_t>();

  if (position + len > end)
    throw std::out_of_range("Read past buffer end");

  const auto value = std::string(position, position + len);
  position += len + 1;
  return value;
}

template <typename T> Reader &operator>>(Reader &r, T &value) {
  value = r.read<T>();
  return r;
}

inline Reader &operator>>(Reader &r, BeegfsFileEventType &value) {
  value = static_cast<BeegfsFileEventType>(
      r.read<std::underlying_type<BeegfsFileEventType>::type>());
  return r;
}

std::string to_string(const BeegfsFileEventType &fileEvent) {
  const static std::map<BeegfsFileEventType, std::string> ops = {
      {BeegfsFileEventType::FLUSH, BeegfsEvents::FLUSH_OPERATION},
      {BeegfsFileEventType::TRUNCATE, BeegfsEvents::TRUNCATE_OPERATION},
      {BeegfsFileEventType::SETATTR, BeegfsEvents::SET_ATTR_OPERATION},
      {BeegfsFileEventType::CLOSE_WRITE, BeegfsEvents::CLOSE_AFTER_WRITE_OPERATION},
      {BeegfsFileEventType::CREATE, BeegfsEvents::CREATE_OPERATION},
      {BeegfsFileEventType::MKDIR, BeegfsEvents::MKDIR_OPERATION},
      {BeegfsFileEventType::MKNOD, BeegfsEvents::MKNOD_OPERATION},
      {BeegfsFileEventType::SYMLINK, BeegfsEvents::SYMLINK_OPERATION},
      {BeegfsFileEventType::RMDIR, BeegfsEvents::RMDIR_OPERATION},
      {BeegfsFileEventType::UNLINK, BeegfsEvents::UNLINK_OPERATION},
      {BeegfsFileEventType::HARDLINK, BeegfsEvents::HARDLINK_OPERATION},
      {BeegfsFileEventType::RENAME, BeegfsEvents::RENAME_OPERATION}};
  auto it = ops.find(fileEvent);
  return it == ops.end() ? std::string() : it->second;
}

std::ostream& operator<<(std::ostream& os, const BeegfsLogPacket& p)
{
   os << to_string(p.type) << " " << p.path;
   return os;
}

BeegfsFileEventLog::BeegfsFileEventLog(const std::string &socketPath, PathFilterFunc pathFilterFunc)
    : _pathFilterFunc(pathFilterFunc) {

  struct sockaddr_un listenAddr;

  listenAddr.sun_family = AF_UNIX;

  strncpy(listenAddr.sun_path, socketPath.c_str(), sizeof(listenAddr.sun_path));
  listenAddr.sun_path[sizeof(listenAddr.sun_path) - 1] = '\0';

  unlink(listenAddr.sun_path);

  listenFD = socket(AF_UNIX, SOCK_SEQPACKET, 0);

  if (listenFD < 0)
    throw exception("Unable to create socket " + socketPath + ": " +
                    std::string(strerror(errno)));

  const auto len = strlen(listenAddr.sun_path) + sizeof(listenAddr.sun_family);
  if (bind(listenFD, (struct sockaddr *)&listenAddr, len) == -1) {
    throw exception("Unable to bind socket:" + std::string(strerror(errno)));
  }

  if (listen(listenFD, 1) == -1) {
    throw exception("Unable to listen: " + std::string(strerror(errno)));
  }
}

std::pair<BeegfsFileEventLog::ReadErrorCode, BeegfsLogPacket>
BeegfsFileEventLog::read() {
  if (serverFD == -1) {
    onAccept = true;
    serverFD = accept(listenFD, NULL, NULL);
    onAccept = false;

    if (serverFD < 0) {
        removeSockets();
        std::pair<BeegfsFileEventLog::ReadErrorCode, BeegfsLogPacket> dummy;
        dummy.first = BeegfsFileEventLog::ReadErrorCode::ReadFailed;
        return dummy;
    }
  }
  const auto recvRes = read_packet(serverFD);

  if (ReadErrorCode::Success != recvRes.first) {
    if (serverFD != -1) {
        close(serverFD);
        serverFD = -1;
    }
  }

  return recvRes;
}

BeegfsFileEventLog::~BeegfsFileEventLog() {
    removeSockets();
}

std::pair<BeegfsFileEventLog::ReadErrorCode, BeegfsLogPacket>
BeegfsFileEventLog::read_packet(int fd) {
  BeegfsLogPacket res;
  char data[65536];
  onReceive = true;
  const ssize_t bytesRead = recv(fd, data, sizeof(data), 0);
  onReceive = false;
  const ssize_t headerSize = sizeof(BeegfsLogPacket::formatVersionMajor) +
                          sizeof(BeegfsLogPacket::formatVersionMinor) +
                          sizeof(BeegfsLogPacket::size);

  if (bytesRead < headerSize) {
    return { ReadErrorCode::ReadFailed, {} };
  }

  Reader reader(&data[0], bytesRead);

  reader >> res.formatVersionMajor >> res.formatVersionMinor;

  if (res.formatVersionMajor != BEEGFS_EVENTLOG_FORMAT_MAJOR ||
      res.formatVersionMinor != BEEGFS_EVENTLOG_FORMAT_MINOR) // should be '<' for future versions
    return {ReadErrorCode::VersionMismatch, {}};

  reader >> res.size;

  if (res.size != bytesRead)
    return {ReadErrorCode::InvalidSize, {}};

  reader >> res.droppedSeq >> res.missedSeq >> res.type >> res.entryId >>
      res.parentEntryId >> res.path >> res.targetPath >> res.targetParentId;

  if (_pathFilterFunc && _pathFilterFunc(res.path)) {
      return {ReadErrorCode::IgnoredPath, {}};
  }
  return {ReadErrorCode::Success, res};
}

void BeegfsFileEventLog::removeSockets() {
    if (serverFD != -1) {
        if (onReceive) {
            shutdown(serverFD, SHUT_RDWR);
        }
        close(serverFD);
        serverFD = -1;
    }
    if (listenFD != -1) {
        if (onAccept) {
            shutdown(listenFD, SHUT_RDWR);
        }
        close(listenFD);
        listenFD=-1;
    }
}
