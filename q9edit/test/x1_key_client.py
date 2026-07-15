#!/usr/bin/env python3
"""Exercise qetermprobe or qe through TCP terminal port 2000."""

import socket
import sys
import time

IAC = 255
DONT = 254
DO = 253
WONT = 252
WILL = 251
SB = 250
SE = 240
ECHO = 1
SUPPRESS_GO_AHEAD = 3


class TelnetStream:
    def __init__(self, host: str, port: int) -> None:
        self.sock = socket.create_connection((host, port), timeout=10)
        self.sock.settimeout(1)
        self.plain = bytearray()
        self.state = "data"
        self.command = 0
        self.output_tail = bytearray()

    def close(self) -> None:
        self.sock.close()

    def send(self, data: bytes) -> None:
        self.sock.sendall(data.replace(b"\xff", b"\xff\xff"))

    def _reply_option(self, command: int, option: int) -> None:
        if command == WILL:
            reply = DO if option in (ECHO, SUPPRESS_GO_AHEAD) else DONT
        elif command == DO:
            reply = WILL if option == SUPPRESS_GO_AHEAD else WONT
        else:
            return
        self.sock.sendall(bytes((IAC, reply, option)))

    def _feed(self, data: bytes) -> None:
        for value in data:
            if self.state == "data":
                if value == IAC:
                    self.state = "iac"
                else:
                    self.plain.append(value)
                    self.output_tail.append(value)
                    if len(self.output_tail) > 4:
                        del self.output_tail[0]
                    if self.output_tail == b"\x1b[6n":
                        self.send(b"\x1b[24;80R")
            elif self.state == "iac":
                if value == IAC:
                    self.plain.append(value)
                    self.state = "data"
                elif value in (DO, DONT, WILL, WONT):
                    self.command = value
                    self.state = "option"
                elif value == SB:
                    self.state = "subneg"
                else:
                    self.state = "data"
            elif self.state == "option":
                self._reply_option(self.command, value)
                self.state = "data"
            elif self.state == "subneg":
                if value == IAC:
                    self.state = "subneg_iac"
            elif self.state == "subneg_iac":
                self.state = "data" if value == SE else "subneg"

    def wait_for(self, marker: bytes, timeout: float) -> bytes:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if marker in self.plain:
                result = bytes(self.plain)
                self.plain.clear()
                return result
            try:
                data = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not data:
                raise RuntimeError("terminal connection closed")
            self._feed(data)
        raise TimeoutError(f"timeout waiting for {marker!r}: {bytes(self.plain)!r}")


def main() -> int:
    editor_mode = len(sys.argv) == 2 and sys.argv[1] == "--editor"
    stream = TelnetStream("127.0.0.1", 2000)
    try:
        logged_in = False
        try:
            stream.wait_for(b"User name?:", 4)
        except TimeoutError:
            if b"$" in stream.plain:
                stream.plain.clear()
                logged_in = True
            else:
                stream.send(b"\r")
                try:
                    stream.wait_for(b"User name?:", 4)
                except TimeoutError:
                    if b"$" not in stream.plain:
                        raise
                    stream.plain.clear()
                    logged_in = True
        if not logged_in:
            stream.send(b"super\r")
            stream.wait_for(b"Password", 8)
            stream.send(b"Al35uUbC\r")
            stream.wait_for(b"$", 15)
        if editor_mode:
            stream.send(b"qe /dd/SYS/startup\r")
            stream.wait_for(b"HELP: Ctrl-S = save | Ctrl-Q = quit", 15)
            stream.send(b"QeZ")
            stream.wait_for(b"QeZ", 10)
            stream.send(b"\x11\x11\x11\x11")
            stream.wait_for(b"$", 10)
            print("qe x1 input: PASS")
            return 0
        stream.send(b"qetermprobe -k\r")
        stream.wait_for(b"send up down left right q", 10)
        for key in (b"\x1b[A", b"\x1b[B", b"\x1b[D", b"\x1b[C", b"q"):
            stream.send(key)
            time.sleep(0.15)
        output = stream.wait_for(b"qetermprobe: keys 1002 1003 1000 1001 113", 10)
        if b"terminal restored" not in output:
            raise RuntimeError(f"terminal restore message missing: {output!r}")
        print("qetermprobe x1 keys: PASS")
        return 0
    except Exception as error:
        print(f"qetermprobe x1 keys: FAIL: {error}", file=sys.stderr)
        return 1
    finally:
        stream.close()


if __name__ == "__main__":
    raise SystemExit(main())
