// include/beman/redis/detail/sender.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_DETAIL_SENDER
#define INCLUDED_BEMAN_REDIS_DETAIL_SENDER

#include <beman/execution/execution.hpp>

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace beman::redis::detail {

namespace ex = ::beman::execution;

template <class... Values>
class failed_sender {
  public:
    using sender_concept = ex::sender_tag;
    using completion_signatures = ex::
        completion_signatures<ex::set_value_t(Values...), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>;

    explicit failed_sender(std::string message)
        : error_(std::make_exception_ptr(std::logic_error(std::move(message)))) {}

    [[nodiscard]] auto get_env() const noexcept -> ex::env<> { return {}; }

    template <class Receiver>
    class operation {
      public:
        using operation_state_concept = ex::operation_state_tag;

        operation(std::exception_ptr error, Receiver receiver)
            : error_(std::move(error)), receiver_(std::move(receiver)) {}

        auto start() & noexcept -> void { ex::set_error(std::move(this->receiver_), this->error_); }

      private:
        std::exception_ptr error_;
        Receiver           receiver_;
    };

    template <class Receiver>
    auto connect(Receiver&& receiver) && noexcept -> operation<std::remove_cvref_t<Receiver>> {
        return operation<std::remove_cvref_t<Receiver>>{std::move(this->error_), std::forward<Receiver>(receiver)};
    }

  private:
    std::exception_ptr error_;
};

class stopped_sender {
  public:
    using sender_concept = ex::sender_tag;
    using completion_signatures =
        ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>;

    [[nodiscard]] auto get_env() const noexcept -> ex::env<> { return {}; }

    template <class Receiver>
    class operation {
      public:
        using operation_state_concept = ex::operation_state_tag;

        explicit operation(Receiver receiver) : receiver_(std::move(receiver)) {}

        auto start() & noexcept -> void { ex::set_stopped(std::move(this->receiver_)); }

      private:
        Receiver receiver_;
    };

    template <class Receiver>
    auto connect(Receiver&& receiver) && noexcept -> operation<std::remove_cvref_t<Receiver>> {
        return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver)};
    }
};

} // namespace beman::redis::detail

#endif // INCLUDED_BEMAN_REDIS_DETAIL_SENDER
