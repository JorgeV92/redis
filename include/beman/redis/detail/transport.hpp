// include/beman/redis/detail/transport.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_TRANSPORT
#define INCLUDED_BEMAN_REDIS_DETAIL_TRANSPORT

#include <beman/redis/config.hpp>
#include <beman/redis/detail/sender.hpp>

#include <beman/net/net.hpp>

#include <cstddef>
#include <string>
#include <utility>

namespace beman::redis::detail {

class transport {
  public:
    explicit transport(config const& cfg) : cfg_(cfg) {}

    auto async_connect() -> failed_sender<> {
        return failed_sender<>("beman.redis transport is a stub; beman.net connect is not wired yet");
    }

    auto async_write(std::string payload) -> failed_sender<std::size_t> {
        (void)payload;
        return failed_sender<std::size_t>("beman.redis transport is a stub; async_write is not wired yet");
    }

    auto async_read_some() -> failed_sender<std::string> {
        return failed_sender<std::string>("beman.redis transport is a stub; async_read_some is not wired yet");
    }

  private:
    config cfg_;
    // TODO: Store beman.net io context/socket/resolver state once those APIs settle.
};

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_TRANSPORT
