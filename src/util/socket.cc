#include <cstring>
#include <stdexcept>

#include "src/util/socket.h"
#include "src/util/syscall.h"

namespace continuity {

namespace util {

UnixDomainSocket::UnixDomainSocket()
    : fd_(CheckSyscall("socket", socket(AF_UNIX, SOCK_STREAM, 0))) {}

UnixDomainSocket::~UnixDomainSocket() { close(fd_); }

void UnixDomainSocket::Listen(const std::string& path, bool remove_existing) {
  if (path.length() >= sizeof(sockaddr_un::sun_path)) {
    throw std::runtime_error("path too long");
  }

  sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), path.size());

  if (remove_existing) {
    if (unlink(path.c_str()) < 0 && errno != ENOENT) {
      throw std::runtime_error("unlink failed: " + std::string(strerror(errno)));
    }
  }

  CheckSyscall("bind", bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
  CheckSyscall("listen", listen(fd_, 16));
}

void UnixDomainSocket::Connect(const std::string& path) {
  if (path.length() >= sizeof(sockaddr_un::sun_path)) {
    throw std::runtime_error("path too long");
  }

  sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), path.size());

  CheckSyscall("connect", connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
}

UnixDomainSocket UnixDomainSocket::Accept() {
  int client_fd = CheckSyscall("accept", accept(fd_, nullptr, nullptr));
  return UnixDomainSocket(client_fd);
}

ssize_t UnixDomainSocket::SendMsg(const struct msghdr* msg, int flags) {
  return CheckSyscall("sendmsg", sendmsg(fd_, msg, flags));
}

ssize_t UnixDomainSocket::RecvMsg(struct msghdr* msg, int flags) {
  return CheckSyscall("recvmsg", recvmsg(fd_, msg, flags));
}

std::pair<UnixDomainSocket, UnixDomainSocket> UnixDomainSocket::CreatePair() {
  int fds[2];
  CheckSyscall("socketpair", socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  return std::make_pair(UnixDomainSocket(fds[0]), UnixDomainSocket(fds[1]));
}

}  // namespace util

}  // namespace continuity