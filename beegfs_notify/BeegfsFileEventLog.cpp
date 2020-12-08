#include "BeegfsFileEventLog.h"
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <unistd.h>

class Reader
{
   public:
      explicit Reader(const char* data, const ssize_t size):
         position(data),
         end(data+size)
      {}

      template<typename T>
      T read()
      {
         return readRaw<T>();
      }

   protected:
      const char* position;
      const char* const end;

      template<typename T>
      T readRaw()
      {

         if (position + sizeof(T) >= end)
            throw std::out_of_range("Read past buffer end");

         const auto x = reinterpret_cast<const T*>(position);
         position += sizeof(T);
         return *x;
      }

};

template<>
inline bool Reader::read<bool>()
{
   return (0 != readRaw<uint8_t>());
}

template<>
inline uint32_t Reader::read<u_int32_t>()
{
   return le32toh(readRaw<uint32_t>());
}

template<>
inline uint64_t Reader::read<u_int64_t>()
{
   return le64toh(readRaw<uint64_t>());
}

template <>
inline std::string Reader::read<std::string>()
{
   const auto len = read<uint32_t>();

   if (position + len > end)
      throw std::out_of_range("Read past buffer end");

   const auto value = std::string(position, position + len);
   position += len + 1;
   return value;
}

template <typename T>
Reader& operator>>(Reader& r, T& value)
{
   value = r.read<T>();
   return r;
}

inline Reader& operator>>(Reader& r, BeegfsFileEventType& value)
{
   value = static_cast<BeegfsFileEventType>(r.read<std::underlying_type<BeegfsFileEventType>::type>());
   return r;
}

std::string to_string(const BeegfsFileEventType &fileEvent)
{
    switch (fileEvent)
    {
    case BeegfsFileEventType::FLUSH:
        return "Flush";
    case BeegfsFileEventType::TRUNCATE:
        return "Truncate";
    case BeegfsFileEventType::SETATTR:
        return "SetAttr";
    case BeegfsFileEventType::CLOSE_WRITE:
        return "CloseAfterWrite";
    case BeegfsFileEventType::CREATE:
        return "Create";
    case BeegfsFileEventType::MKDIR:
        return "MKdir";
    case BeegfsFileEventType::MKNOD:
        return "MKnod";
    case BeegfsFileEventType::SYMLINK:
        return "Symlink";
    case BeegfsFileEventType::RMDIR:
        return "RMdir";
    case BeegfsFileEventType::UNLINK:
        return "Unlink";
    case BeegfsFileEventType::HARDLINK:
        return "Hardlink";
    case BeegfsFileEventType::RENAME:
        return "Rename";
    }
    return "";
}

BeegfsFileEventLog::BeegfsFileEventLog(const std::string &socketPath)
{

    struct sockaddr_un listenAddr;

    listenAddr.sun_family = AF_UNIX;

    strncpy(listenAddr.sun_path, socketPath.c_str(), sizeof(listenAddr.sun_path));
    listenAddr.sun_path[sizeof(listenAddr.sun_path)-1] = '\0';

    unlink(listenAddr.sun_path);

    listenFD = socket(AF_UNIX, SOCK_SEQPACKET, 0);

    if (listenFD < 0)
        throw exception("Unable to create socket " + socketPath + ": "
                        + std::string(strerror(errno)));

    const auto len = strlen(listenAddr.sun_path) + sizeof(listenAddr.sun_family);
    if (bind(listenFD, (struct sockaddr *) &listenAddr, len) == -1)
    {
        throw exception("Unable to bind socket:" + std::string(strerror(errno)));
    }

    if (listen(listenFD, 1) == -1)
    {
        throw exception("Unable to listen: " + std::string(strerror(errno)));
    }
}

std::pair<BeegfsFileEventLog::ReadErrorCode, BeegfsLogPacket> BeegfsFileEventLog::read()
{

    if (serverFD == -1)
    {
        serverFD = accept(listenFD, NULL, NULL);

        if (serverFD < 0)
            throw exception("Error accepting connection: " + std::string(strerror(errno)));
    }

    const auto recvRes = read_packet(serverFD);

    if (ReadErrorCode::Success != recvRes.first)
    {
        close(serverFD);
        serverFD = -1;
    }

    return recvRes;
}

BeegfsFileEventLog::~BeegfsFileEventLog()
{
    close(serverFD);
    close(listenFD);
}

std::pair<BeegfsFileEventLog::ReadErrorCode, BeegfsLogPacket> BeegfsFileEventLog::read_packet(int fd)
{
    BeegfsLogPacket res;
    char data[65536];
    const auto bytesRead = recv(fd, data, sizeof(data), 0);

    const auto headerSize =   sizeof(BeegfsLogPacket::formatVersionMajor)
            + sizeof(BeegfsLogPacket::formatVersionMinor)
            + sizeof(BeegfsLogPacket::size);

    if (bytesRead < headerSize)
        return { ReadErrorCode::ReadFailed, {} };

    Reader reader(&data[0], bytesRead);

    reader >> res.formatVersionMajor
            >> res.formatVersionMinor;

    if (   res.formatVersionMajor != BEEGFS_EVENTLOG_FORMAT_MAJOR
           || res.formatVersionMinor <  BEEGFS_EVENTLOG_FORMAT_MINOR)
        return { ReadErrorCode::VersionMismatch, {} };

    reader >> res.size;

    if (res.size != bytesRead )
        return { ReadErrorCode::InvalidSize, {} };

    reader >> res.droppedSeq
            >> res.missedSeq
            >> res.type
            >> res.entryId
            >> res.parentEntryId
            >> res.path
            >> res.targetPath
            >> res.targetParentId;

    return { ReadErrorCode::Success, res };
}
