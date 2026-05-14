// include/beman/redis/connection.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_CONNECTION
#define INCLUDED_BEMAN_REDIS_CONNECTION

#include <beman/redis/config.hpp>
#include <beman/redis/detail/pending_queue.hpp>

#include <memory>

namespace beman::redis::detail {
class transport;
} // namespace beman::redis::detail

namespace beman::redis {

class connection {
  public:
    explicit connection(config cfg);
    ~connection();

    connection(connection&&) noexcept;
    auto operator=(connection&&) noexcept -> connection&;

    connection(connection const&) = delete;
    auto operator=(connection const&) -> connection& = delete;

    [[nodiscard]] auto get_config() const noexcept -> config const&;

  private:
    config                             cfg_;
    std::unique_ptr<detail::transport> transport_;
    detail::pending_queue              pending_;
    // TODO: socket/transport state ownership belongs in detail::transport.
    // TODO: use pending_ to correlate pipelined requests with parsed responses.
};

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_CONNECTION
