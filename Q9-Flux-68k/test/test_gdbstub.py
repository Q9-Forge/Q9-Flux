#!/usr/bin/env python3
"""End-to-end smoke test for the optional Q9 emulator GDB RSP server."""
import os
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def recv_packet(sock, timeout=5):
    sock.settimeout(timeout)
    payload = bytearray()
    while True:
        ch = sock.recv(1)
        if not ch:
            raise RuntimeError("GDB RSP connection closed")
        if ch == b"+":
            continue
        if ch == b"-":
            raise RuntimeError("emulator rejected RSP packet checksum")
        if ch == b"$":
            break
    while True:
        ch = sock.recv(1)
        if ch == b"#":
            break
        payload.extend(ch)
    sock.recv(2)
    sock.sendall(b"+")
    return payload.decode("ascii")


def command(sock, payload, expect_reply=True):
    body = payload.encode("ascii")
    frame = b"$" + body + b"#" + f"{sum(body) & 0xff:02x}".encode("ascii")
    sock.sendall(frame)
    return recv_packet(sock) if expect_reply else None


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_gdbstub.py path/to/q9.exe")
    emulator = Path(sys.argv[1]).resolve()
    if not emulator.is_file():
        raise SystemExit(f"emulator does not exist: {emulator}")

    with tempfile.TemporaryDirectory(prefix="q9-gdb-rsp-") as td:
        temp = Path(td)
        rom = bytearray(512 * 1024)
        rom[:8] = struct.pack(">II", 0x00010000, 0xFE000010)
        rom[0x10:0x14] = bytes.fromhex("4e7160fc")
        rom_path = temp / "loop.rom"
        rom_path.write_bytes(rom)
        cf_path = temp / "empty.cf"
        cf_path.write_bytes(bytes(4 * 1024 * 1024))

        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        env = os.environ.copy()
        env.update({
            "Q9_GDB_PORT": str(port),
            "Q9_VIDEOBRIDGE_TCP_PORT": "0",
            "Q9_VIDEOBRIDGE_UDP_PORT": "0",
        })
        proc = subprocess.Popen(
            [str(emulator), "--rom", str(rom_path), "--cf", str(cf_path)],
            env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
        )
        sock = None
        try:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                try:
                    sock = socket.create_connection(("127.0.0.1", port), timeout=0.3)
                    break
                except OSError:
                    if proc.poll() is not None:
                        raise RuntimeError("emulator exited before opening its GDB port")
                    time.sleep(0.1)
            if sock is None:
                raise RuntimeError("emulator did not open its GDB port")

            supported = command(sock, "qSupported:multiprocess+;qXfer:features:read+;swbreak+;QStartNoAckMode+")
            assert "qXfer:features:read+" in supported
            assert command(sock, "?").startswith("T05")
            xml = command(sock, "qXfer:features:read:target.xml:0,1000")
            assert xml.startswith("l") and "<architecture>m68k</architecture>" in xml
            assert command(sock, "QStartNoAckMode") == "OK"
            assert command(sock, "vCont?") == "vCont;c;s"
            registers = command(sock, "g")
            assert len(registers) == 18 * 8 and registers.endswith("fe000010")
            changed = "12345678" + registers[8:]
            assert command(sock, "G" + changed) == "OK"
            assert command(sock, "g").startswith("12345678")
            assert command(sock, "m10,4") == "4e7160fc"

            # Expose RAM through the board's documented REMAP I/O trigger.
            command(sock, "mffff8000,1")
            assert command(sock, "M100,4:11223344") == "OK"
            assert command(sock, "m100,4") == "11223344"
            assert command(sock, "Z0,fe000012,2") == "OK"
            command(sock, "vCont;c", expect_reply=False)
            assert recv_packet(sock, timeout=5).startswith("T05")
            command(sock, "c", expect_reply=False)
            assert recv_packet(sock, timeout=5).startswith("T05")  # recurring breakpoint
            command(sock, "vCont;s", expect_reply=False)
            assert recv_packet(sock, timeout=5).startswith("T05")
            assert command(sock, "g").endswith("fe000010")
            assert command(sock, "z0,fe000012,2") == "OK"
            command(sock, "c", expect_reply=False)
            time.sleep(0.02)
            sock.sendall(bytes([3]))
            assert recv_packet(sock, timeout=5).startswith("T05")  # asynchronous interrupt
            print("GDB RSP integration PASS")
        finally:
            if sock:
                sock.close()
            if proc.poll() is None:
                proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            if proc.stderr:
                proc.stderr.close()


if __name__ == "__main__":
    main()
