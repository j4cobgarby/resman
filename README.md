resman
======

A simple system resource reservation manager.

## Usage

Resman, since being rewritten in C, consists of two parts: a background daemon and a client program.

### Client

The `resman` executable is the client program. It's what you use to make reservations, check status,
and perform all other operations.

It has a number of subcommands, invoked like so:

```
Usage: ./resman SUBCOMMAND [OPTION...]
Valid subcommands: [r]un, [t]ime, [c]heck, [d]equeue
```

Here are the options for each subcommand:

```
Usage: ./resman run [OPTION...] COMMAND
Submits a job to resmand.

  -m, --msg=MESSAGE          Description of your job.
  -V, --verbose              Give verbose output.
```
```
Usage: ./resman time [OPTION...] DURATION
Reserves the server for some time.

  -m, --msg=MESSAGE          Explanation for your reservation.
  -V, --verbose              Give verbose output.
```
```
Usage: ./resman check [OPTION...]
View running and queued jobs.

  -n, --count=COUNT          How many queued jobs to view.
  -V, --verbose              Give verbose output
```
```
Usage: ./resman dequeue [OPTION...] JOB_ID
Dequeue a job. JOB_IDs can be found by using the check subcommand.

  -V, --verbose              Give verbose output
```

### Daemon

The background daemon is invoked by running the `resmand` executable. It doesn't take arguments, and it logs to the
systemd journal.

