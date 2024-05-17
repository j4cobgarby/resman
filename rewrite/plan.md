# Plan for rewrite

### Client's point of view

Running a process and attaching to its terminal

```
# As two commands
resman send 'long_process.sh'
resman attach

# Or all at once
resman follow 'long_process.sh'
```

In both of the above cases, resman queues the given command and, when its time
comes, executes it itself (but as if it was the calling user, and with the same
environment variables), inside a new tmux session. This tmux session will be
shared using the `tmux -S ` option.

Once the process ends (not the tmux session but the process in it), resman
takes the next request from the queue and runs that.

Can also allocate the server for a set amount of time, although running a
command is preferred when possible.

```
resman time 1h30m
resman time 10m
resman time 2d
```

Resman's status can be queried.

```
# Status message, with required user interaction if server locked
resman confirm

# Either of the below print nothing but return 0 if unlocked or 1 if locked
resman check
resman
```

And can be forcefully released.

```
# Unlocks and clears queue
resman reset

# Unlocks and starts next item in queue
resman skip
```

On a technical level, when a client queues a command-based job it:

 1) Spawns a new tmux session, using the -S flag to give it a name. The name is
    a 4-byte unique ASCII id, prepended with the string `tmuxresman-`. It may not
    be 'TIME'; this is reserved. It's mandatory that the -S path is in tmp, e.g.
    /tmp/tmuxresman-p3Di.

 2) Sends a message to the `/resman_queue` mqueue with the format:
    - The 4 bytes of the tmux session ID
    - A NULL byte.
    - ASCII string of the client's UID in decimal.
    - A NULL byte.
    - Optional NULL-terminated ASCII string for a "reason" for locking (if not
        needed then just the NULL terminator is required.
    - NULL-terminated ASCII string for the command to run.

    e.g. `p3Di\02\0Running some tests\0python3 my_experiment.py\0`
         `m7gY\01\0\0./a.out\0`

 3) Now the tmux session waits idle until the daemon gets around to starting the
    job, at which point the user may view the output by attaching to the tmux
    session.

Duration based jobs can be achieved by simply using sleep as the command.

### The Daaemon

The daemon keeps its own in-memory queue of job structs, which contain:

 - The tmux session path
 - The job "reason"
 - The command to run
 - The caller's UID

When the daemon wants to execute a job, it:

 1) Constructs a wrapper command like this:
 1) Sends C-c keypress to the tmux session, followed by the job to run, followed by ENTER.
 2) 
