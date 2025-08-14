#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <string>

namespace continuity {

namespace util {

class UnixDomainSocket {
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

  // Create a pair of connected Unix domain sockets.
  static std::pair<UnixDomainSocket, UnixDomainSocket> CreatePair();

 private:
  // Construct from an existing file descriptor. Used by accept.
  UnixDomainSocket(int fd) : fd_(fd) {}

  // File descriptor of the socket.
  int fd_;
};

}  // namespace util

}  // namespace continuity