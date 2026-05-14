// tests/beman/redis/connection.test.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/redis.hpp>

#include <cassert>
#include <exception>
#include <optional>
#include <utility>

namespace ex = beman::execution;
namespace redis = beman::redis;

struct connection_receiver {
    using receiver_concept = ex::receiver_tag;

    std::optional<redis::connection>* conn{};
    std::exception_ptr*               error{};
    bool*                             stopped{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value(redis::connection value) && noexcept -> void { this->conn->emplace(std::move(value)); }
    auto set_error(std::exception_ptr value) && noexcept -> void { *this->error = value; }
    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

struct response_receiver {
    using receiver_concept = ex::receiver_tag;

    bool*               value{};
    std::exception_ptr* error{};
    bool*               stopped{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value(redis::generic_response) && noexcept -> void { *this->value = true; }
    auto set_error(std::exception_ptr value) && noexcept -> void { *this->error = value; }
    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

struct stopped_receiver {
    using receiver_concept = ex::receiver_tag;

    bool* stopped{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value() && noexcept -> void {}
    auto set_error(std::exception_ptr) && noexcept -> void {}
    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

auto main() -> int {
    redis::config cfg;
    cfg.host = "localhost";

    std::optional<redis::connection> conn;
    std::exception_ptr               connect_error;
    bool                             connect_stopped = false;

    auto connect_op = ex::connect(redis::connect(cfg), connection_receiver{&conn, &connect_error, &connect_stopped});
    ex::start(connect_op);

    assert(conn.has_value());
    assert(!connect_error);
    assert(!connect_stopped);
    assert(conn->get_config().host == "localhost");

    redis::request req;
    req.push("PING");

    bool               got_value = false;
    std::exception_ptr exec_error;
    bool               exec_stopped = false;

    auto exec_op =
        ex::connect(redis::exec(*conn, std::move(req)), response_receiver{&got_value, &exec_error, &exec_stopped});
    ex::start(exec_op);

    assert(!got_value);
    assert(exec_error);
    assert(!exec_stopped);

    bool run_stopped = false;
    auto run_op = ex::connect(redis::run(*conn), stopped_receiver{&run_stopped});
    ex::start(run_op);
    assert(run_stopped);
}
