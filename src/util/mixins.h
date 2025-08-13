#pragma once

namespace continuity {

namespace util {

class NonCopyable {
 public:
  NonCopyable(const NonCopyable& other) = delete;
  NonCopyable& operator=(const NonCopyable& other) = delete;
  NonCopyable() = default;
};

class NonCopyMoveable : public NonCopyable {
 public:
  NonCopyMoveable(NonCopyMoveable&& other) = delete;
  NonCopyMoveable& operator=(NonCopyMoveable&& other) = delete;
  NonCopyMoveable() = default;
};

}  // namespace util

}  // namespace continuity