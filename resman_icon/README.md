## Resman Status Notifier

![Example image](example.png)

This little script takes a config file (which has to be in ~/.config/resman_status/hosts.json), and makes corresponding status bar icons to inform of the resman status in various servers. It looks like the above screenshot.

The config file should have content similar to this:

```.json
{
    "hosts": [
        {"addr": "athena.cse.chalmers.se", "port": 2024, "user": "garby", "short_name": "ATH"},
        {"addr": "ithaca.cse.chalmers.se", "port": 2024, "user": "garby", "short_name": "ITH"}
    ]
}
```

### Required packages

Modules which need to be installed are:

 - `paramiko`
 - `PyQt6`

Make a virtual environment and run `pip install paramiko PyQt6`

### Desktop Entry

You can make a desktop entry like the following, and put it in `~/.local/share/applications/resman_status.desktop` or similar. Just make sure you get the path correct.

```
[Desktop Entry]
Version=1.0
Name=Resman Status Checker
Comment=Poll a remote for resman status
Exec=/home/garby/Documents/code/py/resman/resman_icon/.venv/bin/python3 /home/garby/Documents/code/py/resman/resman_icon/main.py
Terminal=false
Type=Application
StartupNotify=true
```
