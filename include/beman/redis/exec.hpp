// include/beman/redis/exec.hpp -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_REDIS_EXEC
#define INCLUDED_BEMAN_REDIS_EXEC

#include <beman/execution/execution.hpp>

#include <beman/redis/connection.hpp>
#include <beman/redis/detail/sender.hpp>
#include <beman/redis/request.hpp>
#include <beman/redis/response.hpp>

#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace beman::redis {

class exec_sender {
  public:
    using sender_concept = ::beman::execution::sender_tag;
    using completion_signatures =
        ::beman::execution::completion_signatures<::beman::execution::set_value_t(generic_response),
                                                  ::beman::execution::set_error_t(std::exception_ptr),
                                                  ::beman::execution::set_stopped_t()>;

    exec_sender(connection& conn, request req) : conn_(&conn), req_(std::move(req)) {}

    [[nodiscard]] auto get_env() const noexcept -> ::beman::execution::env<> { return {}; }

    template <class Receiver>
    class operation {
      public:
        using operation_state_concept = ::beman::execution::operation_state_tag;

        operation(connection* conn, request req, Receiver receiver)
            : conn_(conn), req_(std::move(req)), receiver_(std::move(receiver)) {}

        auto start() & noexcept -> void {
            try {
                if (this->conn_ == nullptr) {
                    throw std::logic_error("beman.redis exec started with no connection");
                }
                if (this->req_.empty()) {
                    throw std::logic_error("beman.redis exec requires a non-empty request");
                }

                auto response = this->conn_->execute(std::move(this->req_));
                ::beman::execution::set_value(std::move(this->receiver_), std::move(response));
            } catch (...) {
                ::beman::execution::set_error(std::move(this->receiver_), std::current_exception());
            }
        }

      private:
        connection* conn_;
        request     req_;
        Receiver    receiver_;
    };

    template <class Receiver>
    auto connect(Receiver&& receiver) && noexcept -> operation<std::remove_cvref_t<Receiver>> {
        return operation<std::remove_cvref_t<Receiver>>{
            this->conn_, std::move(this->req_), std::forward<Receiver>(receiver)};
    }

  private:
    connection* conn_;
    request     req_;
};

[[nodiscard]] inline auto exec(connection& conn, request req) -> exec_sender {
    return exec_sender{conn, std::move(req)};
}

[[nodiscard]] inline auto run(connection& conn) -> detail::stopped_sender {
    (void)conn;
    // TODO: a real implementation should run until stopped and deliver RESP3 push messages.
    return detail::stopped_sender{};
}

} // namespace beman::redis

#endif // INCLUDED_BEMAN_REDIS_EXEC
