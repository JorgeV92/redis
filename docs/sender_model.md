# Sender Model

The public operations are intentionally vocabulary-shaped:

- `connect(config)` completes with `set_value(connection)`.
- `exec(connection&, request)` queues a request and completes later with
  `set_value(generic_response)`.
- `run(connection&)` owns the connection's long-lived I/O loop.

For now, `connect` creates the connection object without opening the socket.
`exec` validates and queues the encoded request. `run` opens the `beman.net`
socket if needed, waits for queued requests, writes request payloads, reads bytes
into the connection buffer, parses complete RESP frames, and completes the
corresponding `exec` receivers.

`run` exits through `set_stopped` when the run receiver's stop token is
requested. Callers that use the synchronous starter examples request stop from
the command response receiver after the expected response arrives.

The local sender types use `beman.execution` directly. They remain intentionally
small while cancellation, reconnect, and push-message behavior are still being
designed.
