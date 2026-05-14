// include/beman/redis/detail/resp_parse.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_RESP_PARSE
#define INCLUDED_BEMAN_REDIS_DETAIL_RESP_PARSE

#include <beman/redis/response.hpp>

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace beman::redis::detail {

enum class parse_error_reason { invalid, incomplete };

class parse_error : public std::runtime_error {
  public:
    explicit parse_error(std::string message, parse_error_reason reason = parse_error_reason::invalid)
        : std::runtime_error(std::move(message)), reason_(reason) {}

    [[nodiscard]] auto reason() const noexcept -> parse_error_reason { return this->reason_; }
    [[nodiscard]] auto incomplete() const noexcept -> bool { return this->reason_ == parse_error_reason::incomplete; }

  private:
    parse_error_reason reason_;
};

struct parse_result {
    generic_response values;
    std::size_t      bytes_consumed{};
};

[[nodiscard]] auto parse_one(std::string_view input) -> std::pair<value, std::size_t>;
[[nodiscard]] auto try_parse_one(std::string_view input) -> std::optional<std::pair<value, std::size_t>>;
[[nodiscard]] auto parse(std::string_view input) -> parse_result;

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_RESP_PARSE
