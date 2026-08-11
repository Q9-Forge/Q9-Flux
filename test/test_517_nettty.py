#!/usr/bin/env python3
"""5.17: Boot-Test fuer die umgezogenen Netz-Terminals (/x1..x8, q9_devtype_nettty) --
verbindet per raw TCP zu Port 2000 (Telnet-CRLF wie in 5.16), loggt sich ein, liest
Datum, listet /dd, loggt sauber aus. Startet den Emulator selbst und beendet ihn danach."""
import socket
import subprocess
import sys
import time

ROM = "/Volumes/SSD1TB/projects/MWOS/OS9/68030/PORTS/Q9/CMDS/BOOTOBJS/ROMBUG/romimage.dev.running.BIN"
IMG = "/Volumes/SSD1TB/projects/Q9/local_images/OS9SYS.claudia-517-devreg.hda"

proc = subprocess.Popen(
    ["./build/native/q9.exe", "--rom", ROM, "--cf", IMG, "--net", "nat"],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1
)

def wait_for(marker, timeout=60):
    deadline = time.time() + timeout
    buf = ""
    while time.time() < deadline:
        line = proc.stdout.readline()
        if not line:
            time.sleep(0.05)
            continue
        buf += line
        sys.stdout.write(line)
        if marker in buf:
            return True
    return False

ok = wait_for("devices online", 60)
if not ok:
    print("FAIL: boot marker not seen")
    proc.terminate()
    sys.exit(1)

time.sleep(3)

s = socket.create_connection(("127.0.0.1", 2000), timeout=15)
s.settimeout(15)

def recv_until(sock, marker, timeout=15):
    sock.settimeout(timeout)
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        buf += chunk
        if marker.encode() in buf:
            return buf
    return buf

time.sleep(1)
s.sendall(b"\r\n")

result = b""
result += recv_until(s, "User name?:")
if b"User name?:" not in result:
    print("FAIL: no login prompt over /x-terminal. Got:", result[-300:])
    proc.terminate(); sys.exit(1)
if b"." in result.split(b"User name?:")[-1][:3]:
    print("FAIL: stray '.' after login prompt (5.16 regression). Got:", result[-50:])
    proc.terminate(); sys.exit(1)

s.sendall(b"super\r\n")
result = recv_until(s, "Password")
s.sendall(b"Al35uUbC\r\n")
result = recv_until(s, "$")
if b"$" not in result:
    print("FAIL: no shell prompt after login. Got:", result[-300:])
    proc.terminate(); sys.exit(1)

s.sendall(b"date\r\n")
result = recv_until(s, "$")
print("date output:", result)

s.sendall(b"dir /dd\r\n")
result = recv_until(s, "$")
if b"CMDS" not in result:
    print("FAIL: dir /dd did not list expected content. Got:", result[-300:])
    proc.terminate(); sys.exit(1)

s.sendall(b"echo hello-5.17-nettty\r\n")
result = recv_until(s, "$")
if b"hello-5.17-nettty" not in result:
    print("FAIL: echo did not roundtrip. Got:", result[-300:])
    proc.terminate(); sys.exit(1)

s.sendall(b"logout\r\n")
result = recv_until(s, "econds", 15)
print("logout output:", result)

s.close()
proc.terminate()
try:
    proc.wait(timeout=5)
except subprocess.TimeoutExpired:
    proc.kill()

print("PASS: /x-terminal login/date/dir/echo/logout roundtrip ok")
