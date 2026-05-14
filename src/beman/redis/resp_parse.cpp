// src/beman/redis/resp_parse.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/resp_parse.hpp>

#include <charconv>
#include <cstddef>
#include <memory>
#include <string_view>

namespace beman::redis::detail {
namespace {

class parser {
  public:
    explicit parser(std::string_view input) : input_(input) {}

    [[nodiscard]] auto position() const noexcept -> std::size_t { return this->position_; }

    auto parse_value() -> value {
        if (this->position_ >= this->input_.size()) {
            throw parse_error("incomplete RESP frame", parse_error_reason::incomplete);
        }

        char const prefix = this->input_[this->position_++];
        switch (prefix) {
        case '+':
            return simple_string{std::string{this->read_line()}};
        case '-':
            return error{std::string{this->read_line()}};
        case ':':
            return integer{this->read_integer_line()};
        case '$':
            return this->parse_bulk_string();
        case '*':
            return this->parse_array();
        default:
            throw parse_error("unknown RESP type prefix");
        }
    }

  private:
    auto read_line() -> std::string_view {
        auto const line_end = this->input_.find("\r\n", this->position_);
        if (line_end == std::string_view::npos) {
            throw parse_error("incomplete RESP line", parse_error_reason::incomplete);
        }

        auto const line = this->input_.substr(this->position_, line_end - this->position_);
        this->position_ = line_end + 2u;
        return line;
    }

    auto read_integer_line() -> long long { return parse_integer(this->read_line()); }

    static auto parse_integer(std::string_view text) -> long long {
        long long   value{};
        auto const* begin = text.data();
        auto const* end = text.data() + text.size();
        auto [ptr, error] = std::from_chars(begin, end, value);
        if (error != std::errc{} || ptr != end) {
            throw parse_error("invalid RESP integer");
        }

        return value;
    }

    auto parse_bulk_string() -> value {
        auto const length = this->read_integer_line();
        if (length == -1) {
            return bulk_string{std::nullopt};
        }
        if (length < -1) {
            throw parse_error("invalid RESP bulk string length");
        }

        auto const string_length = static_cast<std::size_t>(length);
        if (this->position_ + string_length + 2u > this->input_.size()) {
            throw parse_error("incomplete RESP bulk string", parse_error_reason::incomplete);
        }

        auto const text = this->input_.substr(this->position_, string_length);
        this->position_ += string_length;
        if (this->input_.substr(this->position_, 2u) != "\r\n") {
            throw parse_error("RESP bulk string missing trailing CRLF");
        }
        this->position_ += 2u;

        return bulk_string{std::string{text}};
    }

    auto parse_array() -> value {
        auto const length = this->read_integer_line();
        if (length == -1) {
            return std::shared_ptr<array>{};
        }
        if (length < -1) {
            throw parse_error("invalid RESP array length");
        }

        auto result = std::make_shared<array>();
        result->elements.reserve(static_cast<std::size_t>(length));
        for (long long index = 0; index != length; ++index) {
            result->elements.push_back(this->parse_value());
        }

        return result;
    }

    std::string_view input_;
    std::size_t      position_ = 0;
};

} // namespace

auto parse_one(std::string_view input) -> std::pair<value, std::size_t> {
    parser p{input};
    auto   parsed = p.parse_value();
    return {std::move(parsed), p.position()};
}

auto try_parse_one(std::string_view input) -> std::optional<std::pair<value, std::size_t>> {
    try {
        return parse_one(input);
    } catch (parse_error const& error) {
        if (error.incomplete()) {
            return std::nullopt;
        }

        throw;
    }
}

auto parse(std::string_view input) -> parse_result {
    parser       p{input};
    parse_result result;

    while (p.position() != input.size()) {
        result.values.push_back(p.parse_value());
    }

    result.bytes_consumed = p.position();
    return result;
}

} // namespace beman::redis::detail
