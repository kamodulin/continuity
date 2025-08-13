#pragma once

#include <cstring>
#include <stdexcept>
#include <string>

namespace continuity {

namespace util {

inline int CheckSyscall(const std::string& name, int result) {
  if (result < 0) {
    throw std::runtime_error(name + " failed, reason: " + strerror(errno));
  }
  return result;
}

}  // namespace util

}  // namespace continuity