usage: resman [-h] [-d DURATION] [-u USER]
              [-r [REASON ...]] [-x [COMMAND ...]] [-c]
              [-R]

A simple system resource allocation program. Persistent
state stored in /etc/res_lock. This program does not
impose any actual lock on system resource usage; it only
keeps track of which user (if any) has laid claim to it
for the time being, and if applicable how long they have
it reserved for. The purpose of this is to prevent two
people accidentally running experiments at the same
time.

options:
  -h, --help            show this help message and exit
  -d DURATION, --duration DURATION
                        how long to reserve server use
                        for (e.g. 1h30m, 25m, 4h)
  -u USER, --user USER  who is using the server during
                        the reservation? Default is you.
  -r [REASON ...], --reason [REASON ...]
                        what experiment are you running?
                        default is blank. completely
                        optional.
  -x [COMMAND ...], --run [COMMAND ...]
                        if this flag is given, COMMAND
                        is executed and the server is
                        reserved until it finishes. In
                        this case, DURATION is ignored.
  -c, --confirm         interactively tell the user the
                        current status of things. if the
                        server is reserved, they are
                        prompted to press enter to
                        confirm that they understand
                        this.
  -R, --release         unlocks an existing reservation.
                        you should probably only use
                        this if you're the person who
                        made the reservation in the
                        first place, and you want to
                        free the server earlier than
                        your allocated time slot. may
                        also be used if this script
                        crashes :)

Can also be run with no arguments, to check status.
Exits with 0 if not reserved, or 1 if currently
reserved. Just `echo $?` afterwards
