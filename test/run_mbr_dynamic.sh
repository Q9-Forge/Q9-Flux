#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
IMAGE="$ROOT/local_images/OS9SYS.cf_mbr_test.hda"
OS9="/Users/afoe/.local/bin/os9"

if [[ ! -x "$ROOT/build/native/q9.exe" ]]; then
    echo "Fehlt: $ROOT/build/native/q9.exe" >&2
    exit 1
fi
if [[ ! -x "$OS9" ]]; then
    echo "Fehlt: $OS9" >&2
    exit 1
fi

echo "Ersetze mbr im Testimage ..."
"$OS9" del "$IMAGE,CMDS/mbr" >/dev/null 2>&1 || true
"$OS9" copy "$ROOT/os9/mbr/mbr" "$IMAGE,CMDS/mbr"

mkdir -p "$ROOT/test/expect/results"

expect - "$ROOT" <<'EOF'
set timeout 240
set root [lindex $argv 0]
set config [file join $root test cf emu_mbr_raw_test.q9]
set exe [file join $root build native q9.exe]
set log [file join $root test expect results test_mbr_dynamic.log]

log_file -noappend $log
spawn $exe $config

expect "devices online"
sleep 2
send "\r"
expect -re {User name\?:}
send "super\r"
expect -re {Password[^:]*:}
send "Al35uUbC\r"
expect -re {\$ ?$}

send "attr -e -pe /dd/CMDS/BOOTOBJS/d\r"
expect -re {\$ ?$}
send "attr -e -pe /dd/CMDS/mbr\r"
expect -re {\$ ?$}

send "load /dd/CMDS/BOOTOBJS/d\r"
expect -re {\$ ?$}
send "iniz d\r"
expect -re {\$ ?$}
send "load /dd/CMDS/mbr\r"
expect -re {\$ ?$}

send "chd /dd/PROJECTS\r"
expect -re {\$ ?$}
send "mbr /d@ /dd/PROJECTS/pcf.tmpl /dd/PROJECTS/rbf.tmpl\r"
expect -re {Descriptor erzeugt:}
expect -re {\$ ?$}

send "attr -e -pe /dd/PROJECTS/d0\r"
expect -re {\$ ?$}
send "load /dd/PROJECTS/d0\r"
expect -re {\$ ?$}
send "iniz d0\r"
expect -re {\$ ?$}

send "dir -e /d0\r"
expect -re {\$ ?$}
send "free /d0\r"
expect -re {\$ ?$}
send "makdir /d0/MBRTEST\r"
expect -re {\$ ?$}
send "copy /dd/SYS/motd /d0/MBRTEST/MOTD.TXT\r"
expect -re {\$ ?$}
send "list /d0/MBRTEST/MOTD.TXT\r"
expect -re {\$ ?$}

send "\x1d"
expect eof
EOF

echo "MBR-/d0-Test abgeschlossen. Log: $ROOT/test/expect/results/test_mbr_dynamic.log"
