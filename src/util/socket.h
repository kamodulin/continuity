#pragma once

#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#include "src/util/mixins.h"

namespace continuity {

namespace util {

class SocketPair : public NonCopyMoveable {
 public:
  // Creates a pair of connected sockets.
  SocketPair(int domain = AF_UNIX, int type = SOCK_STREAM, int protocol = 0);

  ~SocketPair();

  // Returns the first socket file descriptor.
  int GetFirst() { return fds_[0]; }

  // Returns the second socket file descriptor.
  int GetSecond() { return fds_[1]; }

 private:
  int fds_[2];
};

class UnixDomainSocket : public NonCopyMoveable {
 public:
  // Creates an unbound, unconnected Unix domain socket.
  UnixDomainSocket();

  // Close the socket.
  ~UnixDomainSocket();

  // Bind and listen on a path.
  void Listen(const std::string& path, bool remove_existing = true);

  // Connect to a Unix domain socket at the specified path.
  void Connect(const std::string& path);

  // Accept an incoming connection.
  UnixDomainSocket Accept();

  // Send data over the socket.
  ssize_t SendMsg(const struct msghdr* msg, int flags = 0);

  // Receive data over the socket.
  ssize_t RecvMsg(struct msghdr* msg, int flags = 0);

 private:
  // Construct from an existing file descriptor. Used by accept.
  UnixDomainSocket(int fd) : fd_(fd) {}

  // File descriptor of the socket.
  int fd_;
};

}  // namespace util

}  // namespace continuity