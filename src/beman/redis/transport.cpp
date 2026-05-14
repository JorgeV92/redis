// src/beman/redis/transport.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/transport.hpp>

#include <array>
#include <cerrno>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace beman::redis::detail {
namespace {

constexpr int invalid_socket = -1;

auto close_socket(int& socket) noexcept -> void {
    if (socket != invalid_socket) {
        (void)::close(socket);
        socket = invalid_socket;
    }
}

auto endpoint_name(config const& cfg) -> std::string { return cfg.host + ":" + cfg.port; }

auto last_error() -> std::error_code { return std::error_code(errno, std::system_category()); }

auto make_socket_error(std::error_code error, std::string message) -> std::system_error {
    return std::system_error(error, std::move(message));
}

auto disable_sigpipe(int socket) -> void {
#ifdef SO_NOSIGPIPE
    int enabled = 1;
    (void)::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#else
    (void)socket;
#endif
}

auto set_tcp_no_delay(int socket) -> void {
    int enabled = 1;
    (void)::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));
}

} // namespace

transport::transport(config cfg) : cfg_(std::move(cfg)) {}

transport::~transport() { close_socket(this->socket_); }

transport::transport(transport&& other) noexcept
    : cfg_(std::move(other.cfg_)), socket_(std::exchange(other.socket_, invalid_socket)) {}

auto transport::operator=(transport&& other) noexcept -> transport& {
    if (this != &other) {
        close_socket(this->socket_);
        this->cfg_    = std::move(other.cfg_);
        this->socket_ = std::exchange(other.socket_, invalid_socket);
    }

    return *this;
}

auto transport::connect() -> void {
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* raw_results = nullptr;
    if (auto const rc = ::getaddrinfo(this->cfg_.host.c_str(), this->cfg_.port.c_str(), &hints, &raw_results);
        rc != 0) {
        throw std::runtime_error("failed to resolve Redis endpoint " + endpoint_name(this->cfg_) + ": " +
                                 ::gai_strerror(rc));
    }

    auto results = std::unique_ptr<addrinfo, decltype([](addrinfo* value) { ::freeaddrinfo(value); })>{raw_results};
    std::error_code last_connect_error;

    for (auto* current = results.get(); current != nullptr; current = current->ai_next) {
        int candidate = ::socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (candidate == invalid_socket) {
            last_connect_error = last_error();
            continue;
        }

        int rc = 0;
        do {
            rc = ::connect(candidate, current->ai_addr, current->ai_addrlen);
        } while (rc == -1 && errno == EINTR);

        if (rc == 0) {
            close_socket(this->socket_);
            this->socket_ = candidate;
            disable_sigpipe(this->socket_);
            set_tcp_no_delay(this->socket_);
            return;
        }

        last_connect_error = last_error();
        close_socket(candidate);
    }

    if (last_connect_error) {
        throw make_socket_error(last_connect_error, "failed to connect to Redis endpoint " + endpoint_name(this->cfg_));
    }

    throw std::runtime_error("failed to connect to Redis endpoint " + endpoint_name(this->cfg_) +
                             ": no usable address results");
}

auto transport::write_all(std::string_view payload) -> void {
    if (this->socket_ == invalid_socket) {
        throw std::logic_error("Redis transport is not connected");
    }

    auto const* data = payload.data();
    auto        size = payload.size();

    while (size != 0u) {
#ifdef MSG_NOSIGNAL
        constexpr int flags = MSG_NOSIGNAL;
#else
        constexpr int flags = 0;
#endif
        auto const written = ::send(this->socket_, data, size, flags);
        if (written > 0) {
            auto const written_size = static_cast<std::size_t>(written);
            data += written_size;
            size -= written_size;
            continue;
        }
        if (written == -1 && errno == EINTR) {
            continue;
        }
        if (written == 0) {
            throw std::runtime_error("Redis socket write returned zero bytes");
        }

        throw make_socket_error(last_error(), "failed to write Redis request");
    }
}

auto transport::read_some() -> std::string {
    if (this->socket_ == invalid_socket) {
        throw std::logic_error("Redis transport is not connected");
    }

    std::array<char, 4096> buffer{};
    for (;;) {
        auto const read = ::recv(this->socket_, buffer.data(), buffer.size(), 0);
        if (read > 0) {
            return std::string(buffer.data(), static_cast<std::size_t>(read));
        }
        if (read == 0) {
            return {};
        }
        if (errno == EINTR) {
            continue;
        }

        throw make_socket_error(last_error(), "failed to read Redis response");
    }
}

} // namespace beman::redis::detail
