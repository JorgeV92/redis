// include/beman/redis/error.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_ERROR
#define INCLUDED_BEMAN_REDIS_ERROR

#include <string_view>

namespace beman::redis {

enum class errc {
    ok = 0,
    invalid_request,
    incomplete_frame,
    protocol_error,
    transport_not_available,
};

constexpr auto to_string(errc error) noexcept -> std::string_view {
    switch (error) {
    case errc::ok:
        return "ok";
    case errc::invalid_request:
        return "invalid request";
    case errc::incomplete_frame:
        return "incomplete RESP frame";
    case errc::protocol_error:
        return "Redis protocol error";
    case errc::transport_not_available:
        return "Redis transport is not available";
    }

    return "unknown Redis error";
}

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_ERROR
