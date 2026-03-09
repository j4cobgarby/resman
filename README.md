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
Valid subcommands: [r]un, [t]ime, [c]heck, [d]equeue, [R]elease
```

#### Usage Examples

```sh
# Running a simple command
resman run echo Hello!
resman r echo Hello!        # Short form
resman run ./my_script.sh   # Scripts run directly

# Running a complex command with its own arguments
# If you put a '--' in resman's arguments, everything after it is treated as args to the command.
resman r -- my_program --flag1 --test "A string also" 'anything you want'

# Time duration reservations
resman time 1h
resman t 1m
resman t 45s

# Check current status
resman check
resman c

# Dequeue a job with a specific queue ID (found from `resman check`)
# This just removes it from the middle of the queue.
resman dequeue 10
resman d 5

# Forcefully removes the lock held by the current job and progresses to the next job in the queue,
# or frees the lock entirely if the queue is empty.
resman release
resman R
```

---

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

