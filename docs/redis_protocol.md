# Redis Protocol Notes

The first milestone implements the RESP basics needed for command payload tests:

- simple strings: `+OK\r\n`
- errors: `-ERR message\r\n`
- integers: `:123\r\n`
- bulk strings: `$5\r\nhello\r\n`
- null bulk strings: `$-1\r\n`
- arrays: `*2\r\n...`

RESP3 support is intentionally incomplete. Future work should add maps, sets,
booleans, doubles, verbatim strings, attributes, push messages, and better null
modeling before advertising RESP3 coverage.
