// src/beman/redis/response.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/response.hpp>

#include <string>

namespace beman::redis {
namespace {

struct response_printer {
    auto operator()(simple_string const& item) const -> std::string { return item.value; }

    auto operator()(bulk_string const& item) const -> std::string {
        if (!item.value) {
            return "(nil)";
        }

        return *item.value;
    }

    auto operator()(integer const& item) const -> std::string { return std::to_string(item.value); }

    auto operator()(error const& item) const -> std::string { return "ERR " + item.message; }

    auto operator()(std::shared_ptr<array> const& item) const -> std::string {
        if (!item) {
            return "(null array)";
        }

        std::string out = "[";
        for (std::size_t index = 0; index != item->elements.size(); ++index) {
            if (index != 0u) {
                out += ", ";
            }
            out += to_string(item->elements[index]);
        }
        out += "]";
        return out;
    }
};

} // namespace

auto to_string(value const& response_value) -> std::string { return std::visit(response_printer{}, response_value); }

auto to_string(generic_response const& response) -> std::string {
    std::string out;
    for (std::size_t index = 0; index != response.size(); ++index) {
        if (index != 0u) {
            out += "\n";
        }
        out += to_string(response[index]);
    }
    return out;
}

} // namespace beman::redis
