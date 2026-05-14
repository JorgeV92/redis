// examples/set_get.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/redis.hpp>

#include <exception>
#include <iostream>
#include <optional>
#include <utility>

namespace ex = beman::execution;
namespace redis = beman::redis;

struct capture_connection {
    using receiver_concept = ex::receiver_tag;

    std::optional<redis::connection>* conn{};
    std::exception_ptr*               error{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value(redis::connection value) && noexcept -> void { this->conn->emplace(std::move(value)); }
    auto set_error(std::exception_ptr value) && noexcept -> void { *this->error = value; }
    auto set_stopped() && noexcept -> void {}
};

struct print_result {
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
            std::cout << "SET/GET failed: " << ex.what() << '\n';
        }
    }

    auto set_stopped() && noexcept -> void { std::cout << "SET/GET stopped\n"; }
};

auto main() -> int {
    std::optional<redis::connection> conn;
    std::exception_ptr               error;

    auto connect_op = ex::connect(redis::connect(redis::config{}), capture_connection{&conn, &error});
    ex::start(connect_op);
    if (!conn) {
        return 1;
    }

    redis::request req;
    req.push("SET", "beman.redis:key", "value");
    req.push("GET", "beman.redis:key");

    auto exec_op = ex::connect(redis::exec(*conn, std::move(req)), print_result{});
    ex::start(exec_op);
}
