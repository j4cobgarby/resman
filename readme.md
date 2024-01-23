## usage: resman [-h] [-d DURATION] [-x [COMMAND ...]] [-r [REASON ...]] [-R] [-c] [-u USER]

A simple system resource allocation program. Persistent state stored in /etc/res_lock. This
program does not impose any actual lock on system resource usage; it only keeps track of
which user (if any) has laid claim to it for the time being, and if applicable how long they
have it reserved for. The purpose of this is to prevent two people accidentally running
experiments at the same time.

### options:
 - -h, --help            show this help message and exit
 - -d DURATION, --duration DURATION
                         how long to reserve server use for (e.g. 1h30m, 25m, 4h)
 - -x \[COMMAND ...\], --run \[COMMAND ...\]
                         reserve the server until COMMAND finishes. quotes (') are needed
                         around COMMAND if it contains '-'s, otherwise they're optional.
 - -r \[REASON ...\], --reason \[REASON ...\]
                         what experiment are you running? default is blank. completely
                         optional.
 - -R, --release         unlocks an existing reservation. you should probably only use this
                         if you're the person who made the reservation in the first place,
                         and you want to free the server earlier than your allocated time
                         slot. may also be used if this script crashes :)
 - -c, --confirm         interactively tell the user the current status of things. if the
                         server is reserved, they are prompted to press enter to confirm that
                         they understand this.
 - -u USER, --user USER  who is using the server during the reservation? Default is garby.

Can also be run with no arguments, to check status. Exits with 0 if not reserved, or 1 if
currently reserved. Just `echo $?` afterwards
