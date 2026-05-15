// include/beman/redis/detail/transport.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_TRANSPORT
#define INCLUDED_BEMAN_REDIS_DETAIL_TRANSPORT

#include <beman/redis/config.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace beman::redis::detail {

class transport {
  public:
    explicit transport(config cfg);
    ~transport();

    transport(transport&& other) = delete;
    auto operator=(transport&& other) -> transport& = delete;
    transport(transport const&) = delete;
    auto operator=(transport const&) -> transport& = delete;

    auto connect() -> void;
    auto write_all(std::string_view payload) -> void;
    [[nodiscard]] auto read_some() -> std::string;

  private:
    class impl;

    std::unique_ptr<impl> impl_;
};

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_TRANSPORT
