// examples/ping.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/redis.hpp>

#include <exception>
#include <iostream>
#include <optional>
#include <utility>

namespace ex = beman::execution;
namespace redis = beman::redis;

struct connect_receiver {
    using receiver_concept = ex::receiver_tag;

    std::optional<redis::connection>* conn{};
    std::exception_ptr*               error{};

    auto get_env() const noexcept -> ex::env<> { return {}; }

    auto set_value(redis::connection value) && noexcept -> void { this->conn->emplace(std::move(value)); }
    auto set_error(std::exception_ptr value) && noexcept -> void { *this->error = value; }
    auto set_stopped() && noexcept -> void {}
};

struct print_response_receiver {
    using receiver_concept = ex::receiver_tag;

    auto get_env() const noexcept -> ex::env<> { return {}; }

    auto set_value(redis::generic_response response) && noexcept -> void {
        std::cout << redis::to_string(response) << '\n';
    }

    auto set_error(std::exception_ptr error) && noexcept -> void {
        try {
            if (error) {
                std::rethrow_exception(error);
            }
        } catch (std::exception const& ex) {
            std::cout << "PING failed: " << ex.what() << '\n';
        }
    }

    auto set_stopped() && noexcept -> void { std::cout << "PING stopped\n"; }
};

struct run_receiver {
    using receiver_concept = ex::receiver_tag;

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value() && noexcept -> void {}
    auto set_error(std::exception_ptr error) && noexcept -> void {
        try {
            if (error) {
                std::rethrow_exception(error);
            }
        } catch (std::exception const& ex) {
            std::cout << "run failed: " << ex.what() << '\n';
        }
    }
    auto set_stopped() && noexcept -> void { std::cout << "run stopped\n"; }
};

auto main() -> int {
    redis::config cfg;

    std::optional<redis::connection> conn;
    std::exception_ptr               error;
    auto                             connect_op = ex::connect(redis::connect(cfg), connect_receiver{&conn, &error});
    ex::start(connect_op);

    if (!conn) {
        try {
            if (error) {
                std::rethrow_exception(error);
            }
        } catch (std::exception const& ex) {
            std::cout << "connect failed: " << ex.what() << '\n';
        }
        return 1;
    }

    redis::request req;
    req.push("PING");

    auto exec_op = ex::connect(redis::exec(*conn, std::move(req)), print_response_receiver{});
    ex::start(exec_op);

    auto run_op = ex::connect(redis::run(*conn), run_receiver{});
    ex::start(run_op);
}
