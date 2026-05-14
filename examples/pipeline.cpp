// examples/pipeline.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/resp_parse.hpp>
#include <beman/redis/redis.hpp>

#include <iostream>

namespace redis = beman::redis;

auto main() -> int {
    redis::request req;
    req.push("PING");
    req.push("INCR", "counter");
    req.push("GET", "counter");

    std::cout << "pipeline command count: " << req.size() << '\n';
    std::cout << "encoded bytes: " << req.payload().size() << '\n';

    // Stand-in for bytes that would arrive from Redis after the pipeline.
    auto parsed = redis::detail::parse("+PONG\r\n:1\r\n$1\r\n1\r\n");
    std::cout << redis::to_string(parsed.values) << '\n';
}
