// src/beman/redis/connection.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/connection.hpp>

#include <beman/redis/detail/resp_parse.hpp>
#include <beman/redis/detail/transport.hpp>

#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

namespace beman::redis {

connection::connection(config cfg)
    : cfg_(std::move(cfg)), transport_(std::make_unique<detail::transport>(this->cfg_)) {}

connection::~connection() = default;

connection::connection(connection&& other) noexcept
    : cfg_(std::move(other.cfg_)),
      transport_(std::move(other.transport_)),
      pending_(std::move(other.pending_)),
      input_buffer_(std::move(other.input_buffer_)),
      queued_(std::move(other.queued_)),
      active_(std::move(other.active_)),
      connected_(std::exchange(other.connected_, false)) {}

auto connection::operator=(connection&& other) noexcept -> connection& {
    if (this != &other) {
        std::scoped_lock lock(this->mutex_, other.mutex_);
        this->cfg_          = std::move(other.cfg_);
        this->transport_    = std::move(other.transport_);
        this->pending_      = std::move(other.pending_);
        this->input_buffer_ = std::move(other.input_buffer_);
        this->queued_       = std::move(other.queued_);
        this->active_       = std::move(other.active_);
        this->connected_    = std::exchange(other.connected_, false);
    }

    return *this;
}

auto connection::get_config() const noexcept -> config const& { return this->cfg_; }

auto connection::enqueue(request req, detail::pending_exec_operation& operation) -> void {
    if (req.empty()) {
        throw std::logic_error("beman.redis exec requires a non-empty request");
    }

    {
        std::lock_guard lock(this->mutex_);
        this->queued_.push_back(queued_request{std::move(req), &operation});
    }
    this->queued_cv_.notify_one();
}

auto connection::run(std::function<bool()> stop_requested) -> void {
    try {
        if (!this->connected_) {
            this->transport_->connect();
            this->connected_ = true;
        }
    } catch (...) {
        this->fail_queued(std::current_exception());
        throw;
    }

    try {
        while (!stop_requested()) {
            for (auto& request : this->take_queued(stop_requested)) {
                this->write_request(std::move(request));
            }

            while (!this->active_.empty()) {
                this->dispatch_available_responses();
                if (!this->active_.empty()) {
                    this->read_response_bytes();
                }
            }
        }
    } catch (...) {
        auto error = std::current_exception();
        this->fail_active(error);
        this->fail_queued(error);
        throw;
    }

    this->stop_queued();
}

auto connection::take_queued(std::function<bool()> const& stop_requested) -> std::deque<queued_request> {
    using namespace std::chrono_literals;

    std::unique_lock lock(this->mutex_);
    this->queued_cv_.wait_for(lock, 10ms, [this, &stop_requested] {
        return !this->queued_.empty() || stop_requested();
    });

    std::deque<queued_request> result;
    result.swap(this->queued_);
    return result;
}

auto connection::write_request(queued_request request) -> void {
    auto const expected_responses = request.req.size();
    this->pending_.push(expected_responses);

    active_request active;
    active.expected_responses = expected_responses;
    active.operation          = request.operation;
    active.response.reserve(expected_responses);
    this->active_.push_back(std::move(active));

    this->transport_->write_all(request.req.payload());
}

auto connection::dispatch_available_responses() -> void {
    while (!this->active_.empty()) {
        auto parsed = detail::try_parse_one(this->input_buffer_);
        if (!parsed) {
            return;
        }

        auto& active = this->active_.front();
        active.response.push_back(std::move(parsed->first));
        this->input_buffer_.erase(0u, parsed->second);

        if (active.response.size() == active.expected_responses) {
            auto* operation = active.operation;
            auto  response  = std::move(active.response);
            this->pending_.pop();
            this->active_.pop_front();
            operation->complete(std::move(response));
        }
    }
}

auto connection::read_response_bytes() -> void {
    auto chunk = this->transport_->read_some();
    if (chunk.empty()) {
        throw std::runtime_error("Redis connection closed before a complete response was received");
    }

    this->input_buffer_ += chunk;
}

auto connection::fail_queued(std::exception_ptr error) noexcept -> void {
    std::deque<queued_request> queued;
    {
        std::lock_guard lock(this->mutex_);
        queued.swap(this->queued_);
    }

    while (!queued.empty()) {
        auto current = std::move(queued.front());
        queued.pop_front();
        current.operation->fail(error);
    }
}

auto connection::fail_active(std::exception_ptr error) noexcept -> void {
    while (!this->active_.empty()) {
        auto current = std::move(this->active_.front());
        this->active_.pop_front();
        this->pending_.pop();
        current.operation->fail(error);
    }
}

auto connection::stop_queued() noexcept -> void {
    std::deque<queued_request> queued;
    {
        std::lock_guard lock(this->mutex_);
        queued.swap(this->queued_);
    }

    while (!queued.empty()) {
        auto current = std::move(queued.front());
        queued.pop_front();
        current.operation->stop();
    }
}

} // namespace beman::redis
