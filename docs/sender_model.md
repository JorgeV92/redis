# Sender Model

The public operations are intentionally vocabulary-shaped:

- `connect(config)` completes with `set_value(connection)`.
- `exec(connection&, request)` will complete with `set_value(generic_response)`.
- `run(connection&)` will eventually run until stopped and dispatch push messages.

For now, `connect` opens a TCP socket and `exec` synchronously writes the encoded
request and reads the expected number of RESP frames before completing the
receiver. `run` still completes stopped; the long-lived async reader/writer loop
is the next execution milestone.

The local sender types use `beman.execution` directly. They remain intentionally
small while the run-loop behavior is still being designed.
