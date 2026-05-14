// include/beman/redis/config.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_CONFIG
#define INCLUDED_BEMAN_REDIS_CONFIG

#include <optional>
#include <string>

namespace beman::redis {

struct config {
    std::string                host = "127.0.0.1";
    std::string                port = "6379";
    bool                       use_resp3 = true;
    std::optional<std::string> username;
    std::optional<std::string> password;
};

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_CONFIG
