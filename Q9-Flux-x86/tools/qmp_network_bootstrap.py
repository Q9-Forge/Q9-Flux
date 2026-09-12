#!/usr/bin/env python3
"""Rebind OS-9000/x86 enet0 and start DHCP through a QMP VGA console."""
from __future__ import annotations

import argparse
import json
import socket
import time
from pathlib import Path


def qmp_call(sock: socket.socket, command: str, arguments: dict | None = None) -> None:
    msg: dict[str, object] = {"execute": command}
    if arguments:
        msg["arguments"] = arguments
    sock.sendall((json.dumps(msg) + "\n").encode())
    sock.recv(65536)


def key_event(key: str, down: bool, shift: bool = False) -> dict:
    events = []
    if shift:
        events.append({"type": "key", "data": {"down": True,
                       "key": {"type": "qcode", "data": "shift"}}})
    events.append({"type": "key", "data": {"down": down,
                   "key": {"type": "qcode", "data": key}}})
    if shift and not down:
        events.append({"type": "key", "data": {"down": False,
                       "key": {"type": "qcode", "data": "shift"}}})
    return events


def type_text(sock: socket.socket, text: str, delay: float) -> None:
    for char in text + "\n":
        if char == " ":
            key, shift = "spc", False
        elif char == "/":
            key, shift = "slash", False
        elif char == "&":
            key, shift = "7", True
        elif char.isalpha():
            key, shift = char.lower(), char.isupper()
        elif char.isdigit():
            key, shift = char, False
        else:
            key, shift = {"_": ("minus", True), "-": ("minus", False),
                          "\n": ("ret", False)}[char]
        qmp_call(sock, "input-send-event", {"events": key_event(key, True, shift)})
        qmp_call(sock, "input-send-event", {"events": key_event(key, False, shift)})
        time.sleep(delay)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("qmp_socket", type=Path)
    parser.add_argument("--boot-wait", type=float, default=12.0)
    parser.add_argument("--key-delay", type=float, default=0.2)
    args = parser.parse_args()
    deadline = time.monotonic() + 60
    while not args.qmp_socket.exists() and time.monotonic() < deadline:
        time.sleep(0.2)
    time.sleep(args.boot_wait)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(str(args.qmp_socket))
        sock.recv(65536)
        qmp_call(sock, "qmp_capabilities")
        for command in (
            "ndbmod interface del enet0",
            "ndbmod interface add enet0 binding /spne0/enet",
            "dhcp enet0 -override -v -timeout 3 -tries 1 &",
        ):
            type_text(sock, command, args.key_delay)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
