# Design Notes

`beman.redis` starts with Boost.Redis as architectural inspiration only:

- a move-only connection object
- a request builder that serializes Redis commands
- a generic response value model
- command execution as a queued asynchronous operation
- a future long-lived run loop for responses, pushes, and reconnect handling

The core API is sender-shaped. `connect(config)`, `exec(connection&, request)`,
and `run(connection&)` return `beman.execution` sender-like objects that can be
connected to a receiver and started. The current implementation uses a small
local sender scaffold so protocol work can proceed before the transport is real.

## Milestones

1. Protocol MVP: request encoding, RESP basics, and tests without Redis.
2. Transport MVP: `beman.net` TCP connect/write/read behind `detail::transport`.
3. Execution MVP: command queue, response correlation, and a run loop.
4. Redis MVP: PING, SET, GET, AUTH/HELLO, and basic pipelining against Redis.
5. Resilience MVP: cancellation policy, reconnect state machine, and push messages.

## First Issues

1. Implement `detail::transport` using `beman.net` TCP primitives.
2. Add a connection run loop that reads RESP frames and completes pending commands.
3. Add RESP3 `HELLO` negotiation and optional username/password authentication.
4. Add integration tests gated behind an opt-in live Redis option.
5. Define cancellation and stop-token behavior for `exec` and `run`.

## Research Questions

- Which `beman.net` socket/resolver APIs are intended to be stable enough for
  library code?
- How should `beman.execution` senders expose completion signatures for operations
  that can complete from a background run loop?
- Should `exec` own request payload bytes until write completion or copy into a
  connection-level output buffer?
- How does Boost.Redis split request queuing, response adaptation, and push-message
  dispatch?
- What is the minimal typed response adapter layer that does not complicate the MVP?
