resman
======

A simple system resource reservation manager.

# Usage:

`resman [-h] [-d DURATION] [-x [COMMAND ...]] [-r [REASON ...]] [-R] [-c] [-u USER]`

A simple system resource allocation program. Persistent state stored in /etc/res_lock. *This
program does not impose any actual lock* on system resource usage; it only keeps track of
which user (if any) has laid claim to it for the time being, and if applicable how long they
have it reserved for. The purpose of this is to prevent two people accidentally running
experiments at the same time.

# Options:
| Flag(s) | Description |
|---------|-------------|
|-h, --help| Show this **help message** and exit |
|-d DURATION, --duration DURATION|**How long to reserve** server use for (e.g. 1h30m, 25m, 4h)|
|-x [COMMAND ...], --run [COMMAND ...]|Reserve the server **until COMMAND finishes**. quotes (') are needed around COMMAND if it contains '-'s, otherwise they're optional.|
|-r [REASON ...], --reason [REASON ...]|**What experiment** are you running? Default is blank. Completely optional.|
|-c, --confirm| Interactively **tell the user the current status** of things. If the server is reserved, they are prompted to press enter to confirm they understand this.|
|-u USER, --user USER|**Who is using the server** during the reservation? Default is your username.|

Can also be run with **no arguments**, to check status. Exits with 0 if not reserved, or 1 if
currently reserved. Just `echo $?` afterwards.

# Development

Install development dependencies inside a virtual environment:

```shell
$ python3 -m venv venv
$ source venv/bin/activate

(venv)$ pip install -e .[dev] 
```
