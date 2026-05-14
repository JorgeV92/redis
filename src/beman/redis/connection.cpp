// src/beman/redis/connection.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/connection.hpp>

#include <beman/redis/detail/transport.hpp>

#include <memory>
#include <utility>

namespace beman::redis {

connection::connection(config cfg)
    : cfg_(std::move(cfg)), transport_(std::make_unique<detail::transport>(this->cfg_)) {}

connection::~connection() = default;

connection::connection(connection&&) noexcept = default;

auto connection::operator=(connection&&) noexcept -> connection& = default;

auto connection::get_config() const noexcept -> config const& { return this->cfg_; }

} // namespace beman::redis
