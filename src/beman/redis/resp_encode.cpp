// src/beman/redis/resp_encode.cpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/redis/detail/resp_encode.hpp>

#include <array>
#include <charconv>
#include <stdexcept>

namespace beman::redis::detail {
namespace {

auto append_decimal(std::string& out, std::size_t value) -> void {
    std::array<char, 32> buffer{};
    auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (error != std::errc{}) {
        throw std::runtime_error("failed to encode Redis decimal value");
    }

    out.append(buffer.data(), static_cast<std::size_t>(end - buffer.data()));
}

} // namespace

auto append_array_header(std::string& out, std::size_t element_count) -> void {
    out.push_back('*');
    append_decimal(out, element_count);
    out.append("\r\n");
}

auto append_bulk_string(std::string& out, std::string_view value) -> void {
    out.push_back('$');
    append_decimal(out, value.size());
    out.append("\r\n");
    out.append(value);
    out.append("\r\n");
}

} // namespace beman::redis::detail
