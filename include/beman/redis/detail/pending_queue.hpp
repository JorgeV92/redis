// include/beman/redis/detail/pending_queue.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_PENDING_QUEUE
#define INCLUDED_BEMAN_REDIS_DETAIL_PENDING_QUEUE

#include <cstddef>
#include <deque>
#include <stdexcept>

namespace beman::redis::detail {

struct pending_entry {
    std::size_t request_id{};
    std::size_t expected_responses{};
};

class pending_queue {
  public:
    auto empty() const noexcept -> bool { return this->entries_.empty(); }
    auto size() const noexcept -> std::size_t { return this->entries_.size(); }

    auto push(std::size_t expected_responses) -> pending_entry {
        pending_entry entry{this->next_request_id_++, expected_responses};
        this->entries_.push_back(entry);
        return entry;
    }

    auto front() const -> pending_entry const& {
        if (this->entries_.empty()) {
            throw std::logic_error("pending_queue::front on empty queue");
        }

        return this->entries_.front();
    }

    auto pop() -> void {
        if (this->entries_.empty()) {
            throw std::logic_error("pending_queue::pop on empty queue");
        }

        this->entries_.pop_front();
    }

  private:
    std::deque<pending_entry> entries_;
    std::size_t               next_request_id_ = 1;
};

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_PENDING_QUEUE
