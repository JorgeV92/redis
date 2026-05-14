// examples/coroutine_task_example.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/redis.hpp>

#include <iostream>

namespace redis = beman::redis;

auto main() -> int {
    redis::request req;
    req.push("PING");

    // Intended shape once beman.task integration is added:
    //
    // auto ping(redis::connection& conn) -> beman::task::task<redis::generic_response> {
    //     redis::request req;
    //     req.push("PING");
    //     co_return co_await redis::exec(conn, std::move(req));
    // }
    //
    // The core library intentionally does not depend on coroutines today. A future
    // adapter can make Redis senders awaitable through beman.execution/beman.task.

    std::cout << "coroutine-friendly Redis composition is a TODO; payload bytes: " << req.payload().size() << '\n';
}
