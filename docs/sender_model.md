# Sender Model

The public operations are intentionally vocabulary-shaped:

- `connect(config)` completes with `set_value(connection)`.
- `exec(connection&, request)` will complete with `set_value(generic_response)`.
- `run(connection&)` will eventually run until stopped and dispatch push messages.

For now, `connect` creates a connection object without opening a socket, `exec`
reports a transport-stub error, and `run` completes stopped. This keeps the
receiver flow visible without coupling the protocol layer to unsettled network APIs.

The local sender types use `beman.execution` directly. They remain intentionally
small until transport and run-loop behavior are implemented.
