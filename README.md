# Continuity

Graceful process restarts for C++ applications.

## Overview

Inspired by `tableflip` and `shellflip`, this library aims to provide a simple way to restart C++ network services without dropping existing connections. This is useful for applications that do not reside behind a load balancer or reverse proxy and cannot afford to drop connections during the restart process.

## Features

- [ ] No connections are dropped during an upgrade. After a restart, new connections are only accepted by the new process.
- [ ] Simply run the binary again to restart the process. No need to send signals or use a separate command (could be supported in the future).
- [ ] Check if restart was successful by sending some kind of signal to the old process.
- [ ] Only allow one upgrade at a time.

## Design

The initial process listens on a UNIX domain socket. When a new connection is established, it sends all registered socket file descriptors to the new process over the socket using `sendmsg` with ancillary data of type `SCM_RIGHTS`. The new process receives the file descriptors using `recvmsg` and stores them as a map of socket addresses to file descriptors. Then, as the new process starts, it checks if there are any matching sockets in the received file descriptors. If a match is found, the new process uses the existing socket file descriptor to accept new connections.

## Basic Usage

```cpp
#include "continuity/upgrader.h"

int main() {
    // Create a new socket upgrader instance.
    SocketUpgrader upgrader;

    std::string listen_address = "127.0.0.1:8080";
    int sock = upgrader.get_listener( listen_address );
    if ( sock < 0 ) {
        sock = ...; // Create a new socket, bind it to the address, and listen on it.
        upgrader.register_listener( sock );
    }

    // Set the socket to non-blocking mode to immediately interrupt the listen loop below.
    int flags = ...;
    if (fcntl( sock, F_SETFL, flags | O_NONBLOCK ) < 0) {
        // Handle error.
    }

    // Accept connections on the socket.
    while ( !upgrader.has_completed() ) {
        int client = accept(sock, ...);
        if (client < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            // Handle error.
        }
        // Handle client connection.
    }
}

```

<!-- There are a variety of examples in the `examples` directory that demonstrate how to use the library in different scenarios. For example, we can directly integrate the upgrader into existing code, or wrap `bind` and `accept` calls using link-time interpositioning. -->

<!-- ### Installation

```bash

``` -->

### References

- https://blog.cloudflare.com/oxy-the-journey-of-graceful-restarts/
- https://blog.cloudflare.com/graceful-upgrades-in-go
- https://blog.cloudflare.com/pingora-open-source/
- https://github.blog/news-insights/the-library/glb-part-2-haproxy-zero-downtime-zero-delay-reloads-with-multibinder/
- https://github.com/contribsys/einhorn
