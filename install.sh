#!/usr/bin/bash

cp ./resman.py /usr/bin/resman
echo "Installed successfully at /usr/bin/resman. Make sure /usr/bin is in \$PATH"

touch /etc/res_lock
chmod a+w /etc/res_lock
chmod a+r /etc/res_lock
echo "Lock file = /etc/res_lock"
