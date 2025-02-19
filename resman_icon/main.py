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
    QPixmap,
)
from PyQt6.QtCore import QTimer, Qt, QRect


class StatusIcon:
    def __init__(self, serv_addr: str, port: int, user: str, short_name: str):
        self.serv_addr = serv_addr
        self.port = port
        self.user = user
        self.short_name = short_name

        self.client = paramiko.SSHClient()
        self.client.load_system_host_keys()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        try:
            self.client.connect(
                hostname=self.serv_addr, username=self.user, port=self.port
            )
            print(f"Connected to {self.serv_addr}")
        except Exception as e:
            print(f"Remote connection failed to {self.serv_addr}: {e}")
            return None

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
        self.timer.start(20 * 1000)  # 20s

        self.set_status()

    def overlay_status(self, reserved: bool) -> QIcon:
        pixmap = QPixmap(64, 64)
        pixmap.fill(QColor(20, 20, 20))
        indic_colour = QColor(255, 0, 0) if reserved else QColor(0, 200, 0)
        indic_pad = 10
        indic_height = 15
        font = QFontDatabase.systemFont(QFontDatabase.SystemFont.FixedFont)
        font.setPointSize(22)
        font.setBold(True)

        painter = QPainter(pixmap)
        painter.setBrush(indic_colour)
        painter.setFont(font)

        painter.setPen(indic_colour)
        painter.drawRect(
            QRect(
                indic_pad,
                pixmap.height() - indic_pad - indic_height,
                pixmap.width() - 2 * indic_pad,
                indic_height,
            )
        )

        painter.setPen(QColor(255, 255, 255))
        painter.drawText(
            pixmap.rect().adjusted(0, 0, 0, -pixmap.height() // 3),
            int(Qt.AlignmentFlag.AlignCenter),
            self.short_name,
        )

        painter.end()

        return QIcon(pixmap)

    def poll_server(self):
        try:
            stdin, stdout, stderr = self.client.exec_command("resman")
            status = stdout.channel.recv_exit_status()
            print(f"Got status={status} from {self.serv_addr}")
            return status
        except Exception as e:
            print(f"Remote poll failed: {e}")
            return None

    def set_status(self):
        status = self.poll_server()

        if status is None:
            self.tray_icon.setIcon(QIcon.fromTheme("preferences-system-time"))
            self.tray_icon.setToolTip("Failed to poll...")
            return

        self.tray_icon.setIcon(self.overlay_status(status))
        self.tray_icon.setToolTip(
            f"{'Reserved' if status == 1 else 'Free'} ({self.serv_addr}:{self.port})"
        )

    def open_terminal(self):
        subprocess.Popen(
            [
                "xdg-terminal",
                f"ssh {self.user}@{self.serv_addr} -p {self.port}",
            ],
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
            icn = StatusIcon(
                host["addr"],
                host["port"],
                host["user"],
                host["short_name"],
            )
            if icn:
                self.icns.append(icn)

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
