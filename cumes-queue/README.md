# cumes-queue

`cumes-queue` adds a message to the mail queue.

## How new Messages are Piped into cumes-queue

```
M <reverse-path>
+ <forward-path>
;
RFC_822 Message data
.
```

### Explanation:

The semantics of the *wire protocol* of `cumes-queue` is similar to that of SMTP, except that it's shorter and there are no responses.

SMTP | Wire Protocol
--- | ---
`MAIL FROM:` | `M`
`SEND FROM:` | `S`
`SOML FROM:` | `O`
`SAML FROM:` | `A`
`RCPT TO:` | `+`
`DATA` | `;`

