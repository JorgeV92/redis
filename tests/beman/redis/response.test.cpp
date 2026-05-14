// tests/beman/redis/response.test.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/response.hpp>

#include <cassert>
#include <memory>
#include <optional>

auto main() -> int {
    using namespace beman::redis;

    assert(to_string(value{simple_string{"OK"}}) == "OK");
    assert(to_string(value{bulk_string{std::string{"hello"}}}) == "hello");
    assert(to_string(value{bulk_string{std::nullopt}}) == "(nil)");
    assert(to_string(value{integer{123}}) == "123");
    assert(to_string(value{error{"ERR bad"}}) == "ERR ERR bad");

    auto arr = std::make_shared<array>();
    arr->elements.push_back(simple_string{"one"});
    arr->elements.push_back(integer{2});
    assert(to_string(value{arr}) == "[one, 2]");

    generic_response response;
    response.push_back(simple_string{"PONG"});
    response.push_back(integer{1});
    assert(to_string(response) == "PONG\n1");
}
