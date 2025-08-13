#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <iostream>

#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"

#include "src/util/syscall.h"
#include "src/util/socket.h"

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
  util::SocketPair socket_pair;

  // Start the child process.
  pid_t pid = util::CheckSyscall("fork", fork());
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    execvp(positional_args[1], &positional_args[1]);
  }

  return EXIT_SUCCESS;
}
