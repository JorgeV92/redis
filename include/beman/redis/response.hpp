// include/beman/redis/response.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_RESPONSE
#define INCLUDED_BEMAN_REDIS_RESPONSE

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace beman::redis {

struct simple_string {
    std::string value;

    friend auto operator==(simple_string const&, simple_string const&) -> bool = default;
};

struct bulk_string {
    std::optional<std::string> value;

    friend auto operator==(bulk_string const&, bulk_string const&) -> bool = default;
};

struct integer {
    long long value{};

    friend auto operator==(integer const&, integer const&) -> bool = default;
};

struct error {
    std::string message;

    friend auto operator==(error const&, error const&) -> bool = default;
};

struct array;

using value = std::variant<simple_string, bulk_string, integer, error, std::shared_ptr<array>>;

struct array {
    std::vector<value> elements;
};

using generic_response = std::vector<value>;

[[nodiscard]] auto to_string(value const& response_value) -> std::string;
[[nodiscard]] auto to_string(generic_response const& response) -> std::string;

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_RESPONSE
