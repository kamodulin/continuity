#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <csignal>
#include <iostream>

#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"

#include "src/tracee.h"
#include "src/util/socket.h"
#include "src/util/syscall.h"

using namespace continuity;

ABSL_FLAG(std::string, socket_path, "/tmp/continuity.sock",
          "Path to the Unix domain socket for coordination.");

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage("Usage: continuity <options> [program]");

  auto positional_args = absl::ParseCommandLine(argc, argv);
  if (positional_args.size() < 2) {
    std::cerr << "Error: No program specified to run." << std::endl;
    std::cerr << absl::ProgramUsageMessage() << std::endl;
    return EXIT_FAILURE;
  }

  // Initialize logging
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  std::string socket_path = absl::GetFlag(FLAGS_socket_path);
  LOG(INFO) << "Using socket path: " << socket_path;

  // Create a socket pair for the parent and child to communicate.
  auto [parent_socket, child_socket] = util::UnixDomainSocket::CreatePair();

  // Start the child process.
  pid_t pid = util::CheckSyscall("fork", fork());
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    execvp(positional_args[1], &positional_args[1]);
  }

  {
    Tracee child(pid);

    LOG(INFO) << "Child process started with pid: " << pid;
    child.WaitForSyscall(SYS_bind, true);
    LOG(INFO) << "Child entered the bind syscall";
    child.WaitForSyscall(SYS_bind, false);
    LOG(INFO) << "Child exited the bind syscall";

    // Tracee detached when it falls out of scopre
  }

  kill(pid, SIGKILL);

  // Run the parent logic.

  return EXIT_SUCCESS;
}
