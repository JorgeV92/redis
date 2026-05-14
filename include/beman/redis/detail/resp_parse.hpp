// include/beman/redis/detail/resp_parse.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_RESP_PARSE
#define INCLUDED_BEMAN_REDIS_DETAIL_RESP_PARSE

#include <beman/redis/response.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace beman::redis::detail {

class parse_error : public std::runtime_error {
  public:
    explicit parse_error(std::string message) : std::runtime_error(std::move(message)) {}
};

struct parse_result {
    generic_response values;
    std::size_t      bytes_consumed{};
};

[[nodiscard]] auto parse_one(std::string_view input) -> std::pair<value, std::size_t>;
[[nodiscard]] auto parse(std::string_view input) -> parse_result;

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_RESP_PARSE
