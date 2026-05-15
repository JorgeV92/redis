// include/beman/redis/connection.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_CONNECTION
#define INCLUDED_BEMAN_REDIS_CONNECTION

#include <beman/redis/config.hpp>
#include <beman/redis/detail/pending_queue.hpp>
#include <beman/redis/request.hpp>
#include <beman/redis/response.hpp>

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace beman::redis::detail {
class transport;

class pending_exec_operation {
  public:
    virtual ~pending_exec_operation() = default;

    virtual auto complete(generic_response response) noexcept -> void = 0;
    virtual auto fail(std::exception_ptr error) noexcept -> void      = 0;
    virtual auto stop() noexcept -> void                              = 0;
};
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
    auto               enqueue(request req, detail::pending_exec_operation& operation) -> void;
    auto               run(std::function<bool()> stop_requested) -> void;

  private:
    struct queued_request {
        request                         req;
        detail::pending_exec_operation* operation{};
    };

    struct active_request {
        std::size_t                     expected_responses{};
        detail::pending_exec_operation* operation{};
        generic_response                response;
    };

    auto take_queued(std::function<bool()> const& stop_requested) -> std::deque<queued_request>;
    auto write_request(queued_request request) -> void;
    auto dispatch_available_responses() -> void;
    auto read_response_bytes() -> void;
    auto fail_queued(std::exception_ptr error) noexcept -> void;
    auto fail_active(std::exception_ptr error) noexcept -> void;
    auto stop_queued() noexcept -> void;

    config                             cfg_;
    std::unique_ptr<detail::transport> transport_;
    detail::pending_queue              pending_;
    std::string                        input_buffer_;
    std::deque<queued_request>         queued_;
    std::deque<active_request>         active_;
    std::mutex                         mutex_;
    std::condition_variable            queued_cv_;
    bool                               connected_ = false;
};

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_CONNECTION
