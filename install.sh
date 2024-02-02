#!/usr/bin/bash

cp ./resman.py /usr/bin/resman
echo "Installed successfully at /usr/bin/resman. Make sure /usr/bin is in \$PATH"

touch /etc/res_lock
chmod a+wr /etc/res_lock
echo "Lock file = /etc/res_lock"

touch /etc/res_log
chmod a+wr /etc/res_log
echo "Log file = /etc/res_log"
