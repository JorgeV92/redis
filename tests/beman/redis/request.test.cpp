// tests/beman/redis/request.test.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/request.hpp>

#include <cassert>
#include <string>

auto main() -> int {
    beman::redis::request ping;
    assert(ping.empty());
    assert(ping.size() == 0u);

    ping.push("PING", "hello");
    assert(!ping.empty());
    assert(ping.size() == 1u);
    assert(ping.payload() == "*2\r\n$4\r\nPING\r\n$5\r\nhello\r\n");

    beman::redis::request mixed;
    mixed.push("SET", std::string{"answer"}, 42);
    assert(mixed.payload() == "*3\r\n$3\r\nSET\r\n$6\r\nanswer\r\n$2\r\n42\r\n");

    mixed.push("GET", std::string_view{"answer"});
    assert(mixed.size() == 2u);
    assert(mixed.payload().find("*2\r\n$3\r\nGET\r\n$6\r\nanswer\r\n") != std::string::npos);
}
