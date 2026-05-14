// tests/beman/redis/resp_parse.test.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/resp_parse.hpp>

#include <cassert>
#include <memory>
#include <optional>
#include <variant>

auto main() -> int {
    using namespace beman::redis;

    auto parsed = detail::parse("+OK\r\n-ERR bad\r\n:123\r\n$5\r\nhello\r\n$-1\r\n*2\r\n+one\r\n:2\r\n");
    assert(parsed.bytes_consumed == 51u);
    assert(parsed.values.size() == 6u);

    assert(std::get<simple_string>(parsed.values[0]).value == "OK");
    assert(std::get<error>(parsed.values[1]).message == "ERR bad");
    assert(std::get<integer>(parsed.values[2]).value == 123);
    assert(std::get<bulk_string>(parsed.values[3]).value == "hello");
    assert(!std::get<bulk_string>(parsed.values[4]).value);

    auto arr = std::get<std::shared_ptr<array>>(parsed.values[5]);
    assert(arr);
    assert(arr->elements.size() == 2u);
    assert(std::get<simple_string>(arr->elements[0]).value == "one");
    assert(std::get<integer>(arr->elements[1]).value == 2);

    auto [one, consumed] = detail::parse_one("$3\r\nfoo\r\n+OK\r\n");
    assert(consumed == 9u);
    assert(std::get<bulk_string>(one).value == "foo");

    auto partial = detail::try_parse_one("+O");
    assert(!partial);

    auto complete = detail::try_parse_one("+OK\r\nleftover");
    assert(complete);
    assert(complete->second == 5u);
    assert(std::get<simple_string>(complete->first).value == "OK");

    bool threw = false;
    try {
        (void)detail::parse("$5\r\nhel");
    } catch (detail::parse_error const&) {
        threw = true;
    }
    assert(threw);
}
