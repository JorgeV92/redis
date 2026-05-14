// tests/beman/redis/pending_queue.test.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/pending_queue.hpp>

#include <cassert>
#include <stdexcept>

auto main() -> int {
    beman::redis::detail::pending_queue queue;
    assert(queue.empty());

    auto first = queue.push(1);
    assert(first.request_id == 1u);
    assert(first.expected_responses == 1u);
    assert(queue.size() == 1u);
    assert(queue.front().request_id == 1u);

    auto second = queue.push(3);
    assert(second.request_id == 2u);
    assert(queue.size() == 2u);

    queue.pop();
    assert(queue.front().request_id == 2u);
    queue.pop();
    assert(queue.empty());

    bool threw = false;
    try {
        queue.pop();
    } catch (std::logic_error const&) {
        threw = true;
    }
    assert(threw);
}
