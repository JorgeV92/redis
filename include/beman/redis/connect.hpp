// include/beman/redis/connect.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_CONNECT
#define INCLUDED_BEMAN_REDIS_CONNECT

#include <beman/redis/config.hpp>
#include <beman/redis/connection.hpp>

#include <beman/execution/execution.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace beman::redis {

class connect_sender {
  public:
    using sender_concept = ::beman::execution::sender_tag;
    using completion_signatures =
        ::beman::execution::completion_signatures<::beman::execution::set_value_t(connection),
                                                  ::beman::execution::set_error_t(std::exception_ptr),
                                                  ::beman::execution::set_stopped_t()>;

    explicit connect_sender(config cfg) : cfg_(std::move(cfg)) {}

    [[nodiscard]] auto get_env() const noexcept -> ::beman::execution::env<> { return {}; }

    template <class Receiver>
    class operation {
      public:
        using operation_state_concept = ::beman::execution::operation_state_tag;

        operation(config cfg, Receiver receiver) : cfg_(std::move(cfg)), receiver_(std::move(receiver)) {}

        auto start() & noexcept -> void {
            try {
                connection conn{std::move(this->cfg_)};
                conn.connect();
                ::beman::execution::set_value(std::move(this->receiver_), std::move(conn));
            } catch (...) {
                ::beman::execution::set_error(std::move(this->receiver_), std::current_exception());
            }
        }

      private:
        config   cfg_;
        Receiver receiver_;
    };

    template <class Receiver>
    auto connect(Receiver&& receiver) && noexcept -> operation<std::remove_cvref_t<Receiver>> {
        return operation<std::remove_cvref_t<Receiver>>{std::move(this->cfg_), std::forward<Receiver>(receiver)};
    }

    template <class Receiver>
    auto connect(Receiver&& receiver) const& noexcept -> operation<std::remove_cvref_t<Receiver>> {
        return operation<std::remove_cvref_t<Receiver>>{this->cfg_, std::forward<Receiver>(receiver)};
    }

  private:
    config cfg_;
};

[[nodiscard]] inline auto connect(config cfg) -> connect_sender { return connect_sender{std::move(cfg)}; }

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_CONNECT
