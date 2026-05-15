// src/beman/redis/transport.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/transport.hpp>

#include <beman/net/net.hpp>

#include <array>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <netdb.h>

namespace beman::redis::detail {
namespace {

namespace ex  = ::beman::execution;
namespace net = ::beman::net;

auto endpoint_name(config const& cfg) -> std::string { return cfg.host + ":" + cfg.port; }

auto resolve_endpoints(config const& cfg) -> std::vector<net::ip::tcp::endpoint> {
    // beman.net does not currently expose a stable general-purpose resolver API
    // for host/service names here. Keep this fallback isolated so it can be
    // replaced with the native beman.net resolver once that functionality lands.
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* raw_results = nullptr;
    if (auto const rc = ::getaddrinfo(cfg.host.c_str(), cfg.port.c_str(), &hints, &raw_results); rc != 0) {
        throw std::runtime_error("failed to resolve Redis endpoint " + endpoint_name(cfg) + ": " + ::gai_strerror(rc));
    }

    auto results = std::unique_ptr<addrinfo, decltype([](addrinfo* value) { ::freeaddrinfo(value); })>{raw_results};

    std::vector<net::ip::tcp::endpoint> endpoints;
    for (auto* current = results.get(); current != nullptr; current = current->ai_next) {
        if (current->ai_addr == nullptr) {
            continue;
        }

        net::detail::endpoint endpoint(current->ai_addr, current->ai_addrlen);
        endpoints.emplace_back(endpoint);
    }

    if (endpoints.empty()) {
        throw std::runtime_error("failed to resolve Redis endpoint " + endpoint_name(cfg) + ": no usable addresses");
    }

    return endpoints;
}

auto as_exception(std::error_code error) -> std::exception_ptr {
    return std::make_exception_ptr(std::system_error(error));
}

auto as_exception(std::exception_ptr error) -> std::exception_ptr { return error; }

template <class Error>
auto as_exception(Error&&) -> std::exception_ptr {
    return std::make_exception_ptr(std::runtime_error("beman.net operation failed with an unknown error type"));
}

struct void_receiver {
    using receiver_concept = ex::receiver_tag;

    bool*               done{};
    bool*               stopped{};
    std::exception_ptr* error{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value() && noexcept -> void { *this->done = true; }

    template <class Error>
    auto set_error(Error&& value) && noexcept -> void {
        *this->error = as_exception(std::forward<Error>(value));
    }

    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

template <class T>
struct value_receiver {
    using receiver_concept = ex::receiver_tag;

    std::optional<T>*   value{};
    bool*               stopped{};
    std::exception_ptr* error{};

    auto get_env() const noexcept -> ex::env<> { return {}; }
    auto set_value(T item) && noexcept -> void { this->value->emplace(std::move(item)); }

    template <class Error>
    auto set_error(Error&& item) && noexcept -> void {
        *this->error = as_exception(std::forward<Error>(item));
    }

    auto set_stopped() && noexcept -> void { *this->stopped = true; }
};

template <class Done>
auto run_until_complete(net::io_context&           io,
                        Done                       done,
                        bool const&               stopped,
                        std::exception_ptr const& error,
                        std::string_view          operation) -> void {
    while (!done() && !stopped && !error) {
        if (io.run_one() == 0u) {
            throw std::runtime_error("beman.net " + std::string(operation) + " operation did not complete");
        }
    }

    if (error) {
        std::rethrow_exception(error);
    }
    if (stopped) {
        throw std::runtime_error("beman.net " + std::string(operation) + " operation stopped");
    }
}

auto run_void(net::io_context& io, std::string_view operation, ex::sender auto sender) -> void {
    bool               done    = false;
    bool               stopped = false;
    std::exception_ptr error;

    auto op = ex::connect(std::move(sender), void_receiver{&done, &stopped, &error});
    ex::start(op);
    run_until_complete(io, [&done] { return done; }, stopped, error, operation);
}

template <class T>
auto run_value(net::io_context& io, std::string_view operation, ex::sender auto sender) -> T {
    std::optional<T>   value;
    bool               stopped = false;
    std::exception_ptr error;

    auto op = ex::connect(std::move(sender), value_receiver<T>{&value, &stopped, &error});
    ex::start(op);
    run_until_complete(io, [&value] { return value.has_value(); }, stopped, error, operation);
    return std::move(*value);
}

} // namespace

class transport::impl {
  public:
    explicit impl(config cfg) : cfg_(std::move(cfg)) {}

    auto connect() -> void {
        if (this->connected_) {
            return;
        }

        std::exception_ptr last_error;

        for (auto const& endpoint : resolve_endpoints(this->cfg_)) {
            this->socket_.emplace(this->io_, endpoint);
            try {
                run_void(this->io_, "connect", net::async_connect(*this->socket_));
                this->connected_ = true;
                return;
            } catch (...) {
                last_error = std::current_exception();
                this->socket_.reset();
            }
        }

        if (last_error) {
            std::rethrow_exception(last_error);
        }

        throw std::runtime_error("failed to connect to Redis endpoint " + endpoint_name(this->cfg_));
    }

    auto write_all(std::string_view payload) -> void {
        if (!this->connected_ || !this->socket_) {
            throw std::logic_error("Redis transport is not connected");
        }

        while (!payload.empty()) {
            auto const written = run_value<std::size_t>(
                this->io_, "send", net::async_send(*this->socket_, net::buffer(payload.data(), payload.size())));
            if (written == 0u) {
                throw std::runtime_error("Redis socket write returned zero bytes");
            }

            payload.remove_prefix(written);
        }
    }

    [[nodiscard]] auto read_some() -> std::string {
        if (!this->connected_ || !this->socket_) {
            throw std::logic_error("Redis transport is not connected");
        }

        std::array<char, 4096> buffer{};
        auto const read = run_value<std::size_t>(
            this->io_, "receive", net::async_receive(*this->socket_, net::buffer(buffer.data(), buffer.size())));
        if (read == 0u) {
            return {};
        }

        return std::string(buffer.data(), read);
    }

  private:
    config                                       cfg_;
    net::io_context                              io_;
    std::optional<net::ip::tcp::socket>          socket_;
    bool                                         connected_ = false;
};

transport::transport(config cfg) : impl_(std::make_unique<impl>(std::move(cfg))) {}

transport::~transport() = default;

auto transport::connect() -> void { this->impl_->connect(); }

auto transport::write_all(std::string_view payload) -> void { this->impl_->write_all(payload); }

auto transport::read_some() -> std::string { return this->impl_->read_some(); }

} // namespace beman::redis::detail
