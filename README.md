<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->
# beman.redis: Sender/Receiver Redis Client

![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp26.svg)

`beman.redis` is an experimental Redis client designed around P2300-style
senders and receivers.

**Implements:** experimental Redis client

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## Current Scope

The current MVP supports request encoding and basic RESP2-compatible response
parsing. Networking is isolated behind `beman::redis::detail::transport`, which
uses `getaddrinfo` for endpoint resolution and `beman.net` TCP operations for
connect, write, and read. `exec(connection&, request)` queues work on the
connection and `run(connection&)` drives the queued requests through connect,
write, read, parse, and receiver completion. RESP3 negotiation, authentication,
push-message handling, and a fully long-lived/stoppable run loop are still
pending.

## Build

Preconditions:

- CMake 3.30 or newer
- Ninja or another CMake-supported generator
- A compiler with C++23 support
- `beman.execution`
- `beman.net`

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

Presets are also provided:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

For local development with sibling Beman source checkouts, keep
`execution`, `task`, `net`, and `redis` under the same parent directory and use:

```sh
cmake --preset debug-local-deps
cmake --build --preset debug-local-deps
ctest --preset debug-local-deps
```

## Usage

```cpp
#include <beman/redis/redis.hpp>

namespace redis = beman::redis;

auto make_ping() -> redis::request {
    redis::request req;
    req.push("PING", "hello");
    return req;
}
```

The public asynchronous API is sender-shaped:

```cpp
redis::config cfg;
auto connect_sender = redis::connect(cfg);

// Later, with a receiver:
// auto state = beman::execution::connect(std::move(connect_sender), receiver);
// beman::execution::start(state);
```

Command execution is driven by `run(connection&)`: start one or more `exec`
operation states, then start `run` to process the queued requests.

See `examples/` for complete starter programs.

```bash
./build/debug-local-source-deps/examples/
./build/debug-local-source-deps/examples/beman.redis.examples.ping
./build/debug-local-source-deps/examples/beman.redis.examples.set_get
./build/debug-local-source-deps/examples/beman.redis.examples.pipeline
./build/debug-local-source-deps/examples/beman.redis.examples.coroutine_task_example
```

## Design Goals

- Model asynchronous operations with senders and receivers rather than
  Boost.Asio completion tokens.
- Keep Redis protocol encoding/parsing separate from network transport.

## Roadmap

1. Extend `run(connection&)` into a long-lived stoppable loop that can wait for
   future `exec` requests.
2. Broaden the run loop to support concurrent producers and robust correlation of
   parsed responses.
3. Replace the project-local `getaddrinfo` fallback with a native `beman.net`
   resolver once that API is available.
4. Add authentication and RESP3 `HELLO` negotiation.
5. Add push-message handling for RESP3, pub/sub, and invalidation messages.
6. Define reconnect and cancellation semantics.
7. Add typed response adapters after the generic response model is stable.
