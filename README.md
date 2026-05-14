<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->
# beman.redis: Sender/Receiver Redis Client

![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp26.svg)
![CI](https://github.com/bemanproject/redis/actions/workflows/ci.yml/badge.svg)

`beman.redis` is an experimental Redis client designed around P2300-style
senders and receivers.

**Implements:** experimental Redis client

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## Current Scope

The current MVP supports request encoding and basic RESP2-compatible response
parsing. Networking is deliberately isolated behind `beman::redis::detail::transport`
and currently reports a clear "transport not implemented" error when command
execution is started.

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

1. Replace the transport stub with a `beman.net` TCP implementation.
2. Add a long-lived connection run loop that correlates pending requests with
   parsed responses.
3. Add authentication and RESP3 `HELLO` negotiation.
4. Add push-message handling for RESP3, pub/sub, and invalidation messages.
5. Define reconnect and cancellation semantics.
6. Add typed response adapters after the generic response model is stable.