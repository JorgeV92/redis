// include/beman/redis/request.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_REQUEST
#define INCLUDED_BEMAN_REDIS_REQUEST

#include <beman/redis/detail/resp_encode.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace beman::redis {

class request {
  public:
    request() = default;

    template <class... Args>
    auto push(std::string_view command, Args&&... args) -> void {
        detail::append_array_header(this->payload_, 1u + sizeof...(Args));
        detail::append_bulk(this->payload_, command);
        (detail::append_bulk(this->payload_, std::forward<Args>(args)), ...);
        ++this->command_count_;
    }

    auto payload() const noexcept -> std::string const&;
    auto empty() const noexcept -> bool;
    auto size() const noexcept -> std::size_t;

  private:
    std::string payload_;
    std::size_t command_count_ = 0;
};

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_REQUEST
