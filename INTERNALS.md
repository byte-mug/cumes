# Internals of the cumes mail server

### 1. Overview

Several programs of the cumes suite, that run offer network-services will need a socket-daemon from the `tcphelper` package. The `tcphelper` suite will listen on a *TCP/IP socket* or an *UNIX domain socket* and invoke a specified program for every incoming connection.

In order to enable proper privilege separation, programs do not invoke each other: for example, `cumes-queue` is not invoked by `cumes-smtp` directly. Instead, they connect to a `unixsrv + tcploop` -daemon that will invoke the wanted for every incoming connection. This enables the components to run with their tailored privileges.

Here's the data flow in the cumes suite:

```
cumes-smtp --- cumes-queue --- TODO
```

Every incoming e-mail is added to a central queue directory by `cumes-queue`.

### 2. Queue structure

Each message in the queue is identified by a unique number, let's say 457. The queue is organized into several directories, each of which may contain files related to message 457:

File | Meaning
--- | ---
mess/457 | the message
todo/457 | the envelope: where the message came from, where it's going
intd/457 | the envelope, under construction by qmail-queue
info/457 | the envelope sender address, after preprocessing
local/457 | local envelope recipient addresses, after preprocessing
remote/457 | remote envelope recipient addresses, after preprocessing
bounce/457 | permanent delivery errors

Here are all possible states for a message.
* '+' means a file exists;
* '-' means it does not exist;
* '?' means it may or may not exist.

_ | mess | intd | todo | info | local | remote | bounce | _
--- | --- | --- | --- | --- | --- | --- | --- | ---
   S1. | - | - | - | - | - | - | -
   S2. | + | - | - | - | - | - | -
   S3. | + | + | - | - | - | - | -
   S4. | + | ? | + | ? | ? | ? | - | (queued)
   S5. | + | - | - | + | ? | ? | ? | (preprocessed)

Guarantee: If mess/457 exists, it has inode number 457.

### 3. How messages enter the queue

To add a message to the queue, `cumes-queue` first creates a file in a
separate directory, `pid/`, with a unique name. The filesystem assigns
that file a unique inode number. `cumes-queue` looks at that number, say 457.
By the guarantee above, message 457 must be in state S1.

`cumes-queue` renames the file, it created in `pid/` as `mess/457`, moving to S2. It writes
the message to `mess/457`. It then creates `intd/457`, moving to S3, and
writes the envelope information to `intd/457`.

Finally `cumes-queue` creates a new link, `todo/457`, for `intd/457`, moving
to S4. At that instant the message has been successfully queued, and
`cumes-queue` leaves it for further handling.


