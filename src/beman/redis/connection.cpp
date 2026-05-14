// src/beman/redis/connection.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/connection.hpp>

#include <beman/redis/detail/resp_parse.hpp>
#include <beman/redis/detail/transport.hpp>

#include <memory>
#include <stdexcept>
#include <utility>

namespace beman::redis {

connection::connection(config cfg)
    : cfg_(std::move(cfg)), transport_(std::make_unique<detail::transport>(this->cfg_)) {}

connection::~connection() = default;

connection::connection(connection&&) noexcept = default;

auto connection::operator=(connection&&) noexcept -> connection& = default;

auto connection::get_config() const noexcept -> config const& { return this->cfg_; }

auto connection::connect() -> void { this->transport_->connect(); }

auto connection::execute(request req) -> generic_response {
    if (req.empty()) {
        throw std::logic_error("beman.redis exec requires a non-empty request");
    }

    auto const expected_responses = req.size();
    this->pending_.push(expected_responses);

    try {
        this->transport_->write_all(req.payload());

        generic_response response;
        response.reserve(expected_responses);

        while (response.size() != expected_responses) {
            if (auto parsed = detail::try_parse_one(this->input_buffer_)) {
                response.push_back(std::move(parsed->first));
                this->input_buffer_.erase(0u, parsed->second);
                continue;
            }

            auto chunk = this->transport_->read_some();
            if (chunk.empty()) {
                throw std::runtime_error("Redis connection closed before a complete response was received");
            }
            this->input_buffer_ += chunk;
        }

        this->pending_.pop();
        return response;
    } catch (...) {
        this->pending_.pop();
        throw;
    }
}

} // namespace beman::redis
