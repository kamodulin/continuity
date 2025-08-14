#include <linux/elf.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <cstring>

#include "src/tracee.h"
#include "src/util/syscall.h"

namespace continuity {

Tracee::Tracee(pid_t pid) : pid_(pid), status_(0), in_syscall_ctx_(false), errno_(std::nullopt) {
  Wait();
}

Tracee::~Tracee() { util::CheckSyscall("ptrace", ptrace(PTRACE_DETACH, pid_, 0, 0)); }

template <typename... Args>
long Tracee::ExecuteSyscall(long syscall_number, Args... args) {
  // Ensure that the process is at the entry of a syscall before construcitng the trampoline
  if (!in_syscall_ctx_) {
    WaitForSyscallBoundary();
  }

  // Set the syscall number
  regs_t original_regs = GetRegisters();
  regs_t new_regs = original_regs;
#ifdef __x86_64__
  new_regs.orig_rax = syscall_number;
#elif defined(__aarch64__)
  int syscall_reg = syscall_number;
  struct iovec iov = {
      .iov_base = &syscall_reg,
      .iov_len = sizeof(int),
  };
  ptrace(PTRACE_SETREGSET, pid_, NT_ARM_SYSTEM_CALL, &iov);
#endif

  // Set the system call arguments
  constexpr size_t num_args = sizeof...(Args);
  auto args_array = {args...};

  // TODO(Kamran)

  // Execute the syscall and wait for it to complete
  SetRegisters(&new_regs);
  WaitForSyscallBoundary();

  regs_t result_regs = GetRegisters();
#ifdef __x86_64__
  long result = result_regs.rax;
#elif defined(__aarch64__)
  long result = result_regs.regs[0];
#endif

  if (result < 0) {
    errno_ = -result;
    throw std::runtime_error("remote syscall failed with error: " + std::to_string(errno_.value()));
  } else {
    errno_ = std::nullopt;
  }

  // Restore the original syscall
#ifdef __x86_64__
  original_regs.rip -= 2;
  original_regs.rax = original_regs.orig_rax;
#elif defined(__aarch64__)
  original_regs.pc -= 4;
  iov = {
      .iov_base = &original_regs.regs[8],
      .iov_len = sizeof(int),
  };
  ptrace(PTRACE_SETREGSET, pid_, NT_ARM_SYSTEM_CALL, &iov);
#endif

  SetRegisters(&original_regs);
  WaitForSyscallBoundary();

  return result;
}

void Tracee::WaitForSyscall(long syscall_number, bool entry) {
  auto is_syscall_match = [&]() {
    regs_t regs = GetRegisters();
#ifdef __x86_64__
    return regs.orig_rax == syscall_number;
#elif defined(__aarch64__)
    return regs.regs[8] == syscall_number;
#endif
  };

  // Wait for the syscall to be entered or exited
  while (!is_syscall_match() || in_syscall_ctx_ != entry) {
    WaitForSyscallBoundary();
  }
}

void Tracee::AbortSyscall() {
  if (!in_syscall_ctx_) {
    throw std::runtime_error("cannot abort syscall when not in syscall context");
  }

  // TODO(Kamran)
}

bool Tracee::Exited() const { return WIFEXITED(status_); }

bool Tracee::Stopped() const { return WIFSTOPPED(status_); }

bool Tracee::StoppedWithSignal(int signal) const { return WSTOPSIG(status_) == signal; }

void Tracee::Wait() {
  util::CheckSyscall("waitpid", waitpid(pid_, &status_, 0));

  if (Exited()) {
    throw std::runtime_error("tracee exited abnormally");
  }

  if (!StoppedWithSignal(SIGTRAP)) {
    throw std::runtime_error("tracee did not stop with SIGTRAP");
  }
}

void Tracee::WaitForSyscallBoundary() {
  util::CheckSyscall("ptrace", ptrace(PTRACE_SYSCALL, pid_, nullptr, nullptr));
  Wait();
  in_syscall_ctx_ = !in_syscall_ctx_;
}

regs_t Tracee::GetRegisters() const {
  regs_t regs;
  struct iovec iov = {
      .iov_base = &regs,
      .iov_len = sizeof(regs),
  };
  util::CheckSyscall("ptrace", ptrace(PTRACE_GETREGSET, pid_, NT_PRSTATUS, &iov));
  return regs;
}

void Tracee::SetRegisters(regs_t* regs) {
  struct iovec iov = {
      .iov_base = regs,
      .iov_len = sizeof(*regs),
  };
  util::CheckSyscall("ptrace", ptrace(PTRACE_SETREGSET, pid_, NT_PRSTATUS, &iov));
}

template long Tracee::ExecuteSyscall(long syscall_number, long arg1);
template long Tracee::ExecuteSyscall(long syscall_number, long arg1, long arg2);
template long Tracee::ExecuteSyscall(long syscall_number, long arg1, long arg2, long arg3);
template long Tracee::ExecuteSyscall(long syscall_number, long arg1, long arg2, long arg3,
                                     long arg4);
template long Tracee::ExecuteSyscall(long syscall_number, long arg1, long arg2, long arg3,
                                     long arg4, long arg5);
template long Tracee::ExecuteSyscall(long syscall_number, long arg1, long arg2, long arg3,
                                     long arg4, long arg5, long arg6);

}  // namespace continuity