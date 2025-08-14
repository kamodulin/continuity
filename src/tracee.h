#pragma once

#include <sys/types.h>
#include <optional>

#include "src/util/mixins.h"

#if defined(__x86_64__)
#include <sys/user.h>
typedef struct user_regs_struct regs_t;
#elif defined(__aarch64__)
#include <asm/ptrace.h>
typedef struct user_pt_regs regs_t;
#else
#error "Unsupported architecture: only x86_64 and arm64 are supported."
#endif

namespace continuity {

class Tracee : util::NonCopyMoveable {
 public:
  Tracee(pid_t pid);

  ~Tracee();

  // Perform a syscall in the tracee process, capture the result, and restore
  // the original syscall.
  template <typename... Args>
  long ExecuteSyscall(long syscall_number, Args... args);

  // Wait for a particular syscall to be entered or exited.
  void WaitForSyscall(long syscall_number, bool entry = true);

  // Abort the current syscall.
  void AbortSyscall();

 private:
  // Whether the process has exited.
  bool Exited() const;

  // Whether the process has stopped.
  bool Stopped() const;

  // Whether the process was stopped by the given signal.
  bool StoppedWithSignal(int signal) const;

  // Wait for the process to change state.
  void Wait();

  // Wait for the process to enter or exit a syscall.
  void WaitForSyscallBoundary();

  // Get the process's current register state.
  regs_t GetRegisters() const;

  // Set the process's current register state.
  void SetRegisters(regs_t* regs);

  pid_t pid_;
  int status_;
  bool in_syscall_ctx_;
  std::optional<int> errno_;
};

}  // namespace continuity