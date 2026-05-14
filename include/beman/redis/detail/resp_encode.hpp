// include/beman/redis/detail/resp_encode.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_RESP_ENCODE
#define INCLUDED_BEMAN_REDIS_DETAIL_RESP_ENCODE

#include <charconv>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace beman::redis::detail {

auto append_array_header(std::string& out, std::size_t element_count) -> void;
auto append_bulk_string(std::string& out, std::string_view value) -> void;

namespace resp_encode_detail {

template <class>
inline constexpr bool dependent_false = false;

inline auto as_string_view(std::string_view value) noexcept -> std::string_view { return value; }
inline auto as_string_view(std::string const& value) noexcept -> std::string_view { return value; }
inline auto as_string_view(char const* value) noexcept -> std::string_view { return value; }

template <std::size_t Size>
auto as_string_view(char const (&value)[Size]) noexcept -> std::string_view {
    return std::string_view{value, Size - 1u};
}

template <class T>
concept string_like = requires(T&& value) {
    { as_string_view(std::forward<T>(value)) } -> std::same_as<std::string_view>;
};

template <class T>
concept integer_like = std::integral<std::remove_cvref_t<T>> && !std::same_as<std::remove_cvref_t<T>, bool>;

} // namespace resp_encode_detail

template <class T>
auto append_bulk(std::string& out, T&& value) -> void {
    using value_type = std::remove_cvref_t<T>;

    if constexpr (resp_encode_detail::string_like<T>) {
        append_bulk_string(out, resp_encode_detail::as_string_view(std::forward<T>(value)));
    } else if constexpr (resp_encode_detail::integer_like<T>) {
        char buffer[64]{};
        auto [end, error] = std::to_chars(std::begin(buffer), std::end(buffer), static_cast<value_type>(value));
        if (error != std::errc{}) {
            throw std::runtime_error("failed to encode Redis integer argument");
        }
        append_bulk_string(out, std::string_view{buffer, static_cast<std::size_t>(end - std::begin(buffer))});
    } else {
        static_assert(resp_encode_detail::dependent_false<value_type>, "unsupported Redis request argument type");
    }
}

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_RESP_ENCODE
