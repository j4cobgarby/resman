import sys
import os
import subprocess
import paramiko
import json
from PyQt6.QtWidgets import QApplication, QSystemTrayIcon, QMenu
from PyQt6.QtGui import (
    QIcon,
    QAction,
    QPainter,
    QColor,
    QFontDatabase,
)
from PyQt6.QtCore import QTimer, Qt


class StatusIcon:
    def __init__(self, serv_addr: str, port: int, user: str, short_name: str):
        self.tray_icon = QSystemTrayIcon(
            QIcon.fromTheme("preferences-system-time")
        )

        self.menu = QMenu()

        self.ssh_action = QAction("Open terminal and connect")
        self.ssh_action.triggered.connect(self.open_terminal)
        self.menu.addAction(self.ssh_action)

        self.menu.addSeparator()

        self.quitall_action = QAction("Close all indicators")
        self.quitall_action.triggered.connect(self.exit_app)
        self.menu.addAction(self.quitall_action)

        self.quitme_action = QAction("Close this indicator only")
        self.quitme_action.triggered.connect(self.killme)
        self.menu.addAction(self.quitme_action)

        self.tray_icon.setContextMenu(self.menu)
        self.tray_icon.show()

        self.timer = QTimer()
        self.timer.timeout.connect(self.set_status)
        self.timer.start(10000)

        self.serv_addr = serv_addr
        self.port = port
        self.user = user
        self.short_name = short_name

        self.set_status()

    def overlay_status(self, icon_name: str, reserved: bool) -> QIcon:
        base = QIcon.fromTheme(icon_name)
        px = base.pixmap(64, 64)
        rad = 20
        colour = QColor(255, 0, 0) if reserved else QColor(0, 200, 0)
        fnt = QFontDatabase.systemFont(QFontDatabase.SystemFont.FixedFont)
        fnt.setPointSize(22)
        fnt.setBold(True)

        painter = QPainter(px)
        painter.setPen(QColor(255, 255, 255))
        painter.setBrush(colour)
        painter.drawEllipse(
            px.width() // 2 - rad // 2, px.height() - rad - 5, rad, rad
        )

        painter.setFont(fnt)
        painter.setPen(QColor(255, 255, 255))
        painter.drawText(
            px.rect().adjusted(0, 0, 0, -px.height() // 3),
            int(Qt.AlignmentFlag.AlignCenter),
            self.short_name,
        )
        painter.end()

        return QIcon(px)

    def poll_server(self):
        client = paramiko.SSHClient()
        client.load_system_host_keys()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        try:
            client.connect(
                hostname=self.serv_addr, username=self.user, port=self.port
            )
            stdin, stdout, stderr = client.exec_command("resman")
            status = stdout.channel.recv_exit_status()
            return status
        except Exception as e:
            print(f"Remote poll failed: {e}")
        finally:
            client.close()

    def set_status(self, icn="preferences-system-network-server"):
        resvd = self.poll_server() == 1
        self.tray_icon.setIcon(self.overlay_status(icn, resvd))
        self.tray_icon.setToolTip(
            f"{'Reserved' if resvd else 'Free'} ({self.serv_addr}:{self.port})"
        )

    def open_terminal(self):
        subprocess.Popen(
            [
                "xdg-terminal",
                f"ssh {self.user}@{self.serv_addr} -p {self.port}",
            ]
        )

    def exit_app(self):
        sys.exit()

    def killme(self):
        self.tray_icon.hide()


class StatusApp:
    def __init__(self):
        host_info = self.read_config()
        self.app = QApplication(sys.argv)
        self.icns = []

        # Reversed adding because, on my machine at least, this displays them
        # in the order they're given in the JSON file
        for host in reversed(host_info):
            self.icns.append(
                StatusIcon(
                    host["addr"],
                    host["port"],
                    host["user"],
                    host["short_name"],
                )
            )

    def read_config(self):
        try:
            with open(
                os.path.expanduser("~/.config/resman_status/hosts.json")
            ) as fp:
                conf = json.load(fp)
                if "hosts" not in conf:
                    print("'hosts' key not found in JSON config. Quitting.")
                    sys.exit(-1)
                return conf["hosts"]
        except FileNotFoundError:
            print("Could not find JSON config file. Quitting.")
            sys.exit(-1)

    def run(self):
        self.app.exec()


if __name__ == "__main__":
    tray_app = StatusApp()
    tray_app.run()
