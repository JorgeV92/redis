// src/beman/redis/request.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/request.hpp>

namespace beman::redis {

auto request::payload() const noexcept -> std::string const& { return this->payload_; }

auto request::empty() const noexcept -> bool { return this->command_count_ == 0u; }

auto request::size() const noexcept -> std::size_t { return this->command_count_; }

} // namespace beman::redis
