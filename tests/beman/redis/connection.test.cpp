// tests/beman/redis/connection.test.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/redis.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <variant>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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

    std::optional<redis::generic_response>* value{};
    std::exception_ptr* error{};
    bool*               stopped{};
    ex::inplace_stop_source* source{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value(redis::generic_response response) && noexcept -> void {
        this->value->emplace(std::move(response));
        if (this->source) {
            this->source->request_stop();
        }
    }
    auto set_error(std::exception_ptr value) && noexcept -> void {
        *this->error = value;
        if (this->source) {
            this->source->request_stop();
        }
    }
    auto set_stopped() && noexcept -> void {
        *this->stopped = true;
        if (this->source) {
            this->source->request_stop();
        }
    }
};

struct stopped_receiver {
    using receiver_concept = ex::receiver_tag;

    bool* stopped{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value() && noexcept -> void {}
    auto set_error(std::exception_ptr) && noexcept -> void {}
    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

struct run_receiver {
    using receiver_concept = ex::receiver_tag;

    struct env {
        ex::inplace_stop_source* source{};

        auto query(ex::get_stop_token_t) const noexcept { return this->source->get_token(); }
    };

    bool*               value{};
    std::exception_ptr* error{};
    bool*               stopped{};
    ex::inplace_stop_source* source{};

    auto get_env() const noexcept -> env { return {this->source}; }
    auto set_value() && noexcept -> void { *this->value = true; }
    auto set_error(std::exception_ptr value) && noexcept -> void { *this->error = value; }
    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

namespace {

constexpr int invalid_socket = -1;

auto close_socket(int& socket) noexcept -> void {
    if (socket != invalid_socket) {
        (void)::close(socket);
        socket = invalid_socket;
    }
}

class fake_redis_server {
  public:
    explicit fake_redis_server(std::string response) : response_(std::move(response)) {
        this->listen_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(this->listen_socket_ != invalid_socket);

        int reuse = 1;
        (void)::setsockopt(this->listen_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in address{};
#ifdef __APPLE__
        address.sin_len = sizeof(address);
#endif
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port        = 0;

        assert(::bind(this->listen_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
        assert(::listen(this->listen_socket_, 1) == 0);

        socklen_t address_size = sizeof(address);
        assert(::getsockname(this->listen_socket_, reinterpret_cast<sockaddr*>(&address), &address_size) == 0);
        this->port_ = ntohs(address.sin_port);

        this->thread_ = std::thread([this] { this->serve(); });
    }

    ~fake_redis_server() {
        close_socket(this->listen_socket_);
        if (this->thread_.joinable()) {
            this->thread_.join();
        }
    }

    fake_redis_server(fake_redis_server const&) = delete;
    auto operator=(fake_redis_server const&) -> fake_redis_server& = delete;

    [[nodiscard]] auto port() const noexcept -> unsigned short { return this->port_; }

  private:
    auto serve() -> void {
        int client = ::accept(this->listen_socket_, nullptr, nullptr);
        if (client == invalid_socket) {
            return;
        }

        std::array<char, 1024> request{};
        (void)::recv(client, request.data(), request.size(), 0);

        auto const* data = this->response_.data();
        auto        size = this->response_.size();
        while (size != 0u) {
            auto const written = ::send(client, data, size, 0);
            if (written <= 0) {
                break;
            }
            auto const written_size = static_cast<std::size_t>(written);
            data += written_size;
            size -= written_size;
        }

        (void)::close(client);
    }

    std::string response_;
    int         listen_socket_ = invalid_socket;
    unsigned short port_{};
    std::thread thread_;
};

} // namespace

auto test_connect_and_empty_exec() -> void {
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

    std::optional<redis::generic_response> got_value;
    std::exception_ptr                     exec_error;
    bool                                   exec_stopped = false;

    auto exec_op =
        ex::connect(redis::exec(*conn, std::move(req)), response_receiver{&got_value, &exec_error, &exec_stopped});
    ex::start(exec_op);

    assert(!got_value);
    assert(exec_error);
    assert(!exec_stopped);
}

auto test_live_tcp_ping() -> void {
    fake_redis_server server("+PONG\r\n");

    redis::config cfg;
    cfg.host = "localhost";
    cfg.port = std::to_string(server.port());

    std::optional<redis::connection> conn;
    std::exception_ptr               connect_error;
    bool                             connect_stopped = false;

    auto connect_op = ex::connect(redis::connect(cfg), connection_receiver{&conn, &connect_error, &connect_stopped});
    ex::start(connect_op);

    assert(conn.has_value());
    assert(!connect_error);
    assert(!connect_stopped);
    assert(conn->get_config().host == "localhost");

    std::optional<redis::generic_response> got_value;
    std::exception_ptr exec_error;
    bool               exec_stopped = false;
    ex::inplace_stop_source source;

    bool               run_value = false;
    std::exception_ptr run_error;
    bool               run_stopped = false;
    auto run_op = ex::connect(redis::run(*conn), run_receiver{&run_value, &run_error, &run_stopped, &source});
    std::thread run_thread([&run_op] { ex::start(run_op); });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    redis::request req;
    req.push("PING");

    auto exec_op =
        ex::connect(redis::exec(*conn, std::move(req)), response_receiver{&got_value, &exec_error, &exec_stopped, &source});
    ex::start(exec_op);

    run_thread.join();

    assert(!run_value);
    assert(!run_error);
    assert(run_stopped);

    assert(got_value);
    assert(got_value->size() == 1u);
    assert(std::get<redis::simple_string>((*got_value)[0]).value == "PONG");
    assert(!exec_error);
    assert(!exec_stopped);
}

auto main() -> int {
    if (std::getenv("BEMAN_REDIS_TEST_LIVE_TCP") != nullptr) {
        test_live_tcp_ping();
        return 0;
    }

    test_connect_and_empty_exec();
}
