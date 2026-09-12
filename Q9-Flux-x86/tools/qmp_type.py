#!/usr/bin/env python3
"""Send printable text to a QEMU VGA console through its QMP socket.

OS-9000/x86 uses the VGA terminal by default; its COM1 output is not an
interactive shell.  This helper keeps smoke-test input independent of a
desktop VNC/SDL client.
"""
from __future__ import annotations

import argparse
import json
import socket
import time
from pathlib import Path


KEYS = {
    " ": "spc",
    "-": "minus",
    "=": "equal",
    "/": "slash",
    ".": "dot",
    ",": "comma",
    "\n": "ret",
}
SHIFTED_KEYS = {
    "@": "2",
    "&": "7",
    ":": "semicolon",
    "<": "comma",
    ">": "dot",
    "_": "minus",
    "?": "slash",
}


def key_for(character: str) -> tuple[str, bool]:
    """Return a QMP qcode and whether it requires Shift."""
    if "A" <= character <= "Z":
        return character.lower(), True
    if character in SHIFTED_KEYS:
        return SHIFTED_KEYS[character], True
    if character in KEYS:
        return KEYS[character], False
    if character.isascii() and character.isalnum():
        return character, False
    raise ValueError(character)


def qmp_call(connection: socket.socket, command: str, arguments: dict | None = None) -> dict:
    message: dict[str, object] = {"execute": command}
    if arguments:
        message["arguments"] = arguments
    connection.sendall((json.dumps(message) + "\n").encode("ascii"))
    response = json.loads(connection.recv(65536))
    if "error" in response:
        raise RuntimeError(response["error"])
    return response


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("qmp_socket", type=Path)
    parser.add_argument("text", nargs="?", help="text to type; append Enter with --enter")
    parser.add_argument("--enter", action="store_true", help="send Enter after text")
    parser.add_argument("--ctrl-c", action="store_true", help="send the interrupt key chord Ctrl-C")
    parser.add_argument("--key-delay", type=float, default=0.03,
                        help="delay in seconds between keystrokes (default: 0.03)")
    args = parser.parse_args()
    if args.key_delay < 0:
        parser.error("--key-delay must not be negative")
    if args.text is None and not args.ctrl_c:
        parser.error("text or --ctrl-c is required")
    if args.text is not None and args.ctrl_c:
        parser.error("--ctrl-c cannot be combined with text")

    typed = (args.text or "") + ("\n" if args.enter else "")
    unsupported = sorted({character for character in typed
                          if not (character.isascii() and
                                  (character.isalnum() or character in KEYS or character in SHIFTED_KEYS))})
    if unsupported:
        parser.error(f"unsupported characters: {''.join(unsupported)!r}")

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.connect(str(args.qmp_socket))
        connection.recv(65536)  # QMP greeting
        qmp_call(connection, "qmp_capabilities")
        def send_key(key: str, shift: bool = False) -> None:
            events = []
            if shift:
                events.append({"type": "key", "data": {"down": True,
                                                            "key": {"type": "qcode", "data": "shift"}}})
            events += [
                {"type": "key", "data": {"down": True,
                                               "key": {"type": "qcode", "data": key}}},
                {"type": "key", "data": {"down": False,
                                               "key": {"type": "qcode", "data": key}}},
            ]
            if shift:
                events.append({"type": "key", "data": {"down": False,
                                                            "key": {"type": "qcode", "data": "shift"}}})
            qmp_call(connection, "input-send-event", {"events": events})
        if args.ctrl_c:
            key_event = lambda down, key: {"type": "key", "data": {
                "down": down, "key": {"type": "qcode", "data": key}}}
            qmp_call(connection, "input-send-event", {"events": [
                key_event(True, "ctrl"), key_event(True, "c"),
                key_event(False, "c"), key_event(False, "ctrl"),
            ]})
            return 0
        for character in typed:
            key, shift = key_for(character)
            send_key(key, shift)
            time.sleep(args.key_delay)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
