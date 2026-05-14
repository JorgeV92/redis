// include/beman/redis/connection.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_CONNECTION
#define INCLUDED_BEMAN_REDIS_CONNECTION

#include <beman/redis/config.hpp>
#include <beman/redis/detail/pending_queue.hpp>
#include <beman/redis/request.hpp>
#include <beman/redis/response.hpp>

#include <memory>
#include <string>

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
    auto               connect() -> void;
    auto               execute(request req) -> generic_response;

  private:
    config                             cfg_;
    std::unique_ptr<detail::transport> transport_;
    detail::pending_queue              pending_;
    std::string                        input_buffer_;
};

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_CONNECTION
