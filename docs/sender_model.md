# Sender Model

The public operations are intentionally vocabulary-shaped:

- `connect(config)` completes with `set_value(connection)`.
- `exec(connection&, request)` queues a request and completes later with
  `set_value(generic_response)`.
- `run(connection&)` drives the connection's queued work.

For now, `connect` creates the connection object without opening the socket.
`exec` validates and queues the encoded request. `run` opens the `beman.net`
socket if needed, writes queued request payloads, reads RESP frames, and
completes the corresponding `exec` receivers.

The current `run` drains work that is already queued and then completes. A
long-lived stoppable loop that can wait for future `exec` requests is the next
execution milestone.

The local sender types use `beman.execution` directly. They remain intentionally
small while the long-lived run-loop behavior is still being designed.
