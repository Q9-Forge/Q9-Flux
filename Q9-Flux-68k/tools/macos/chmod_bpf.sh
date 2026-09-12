#!/bin/sh
# Wird von com.q9.chmod-bpf.plist (LaunchDaemon) bei jedem Boot ausgefuehrt: macOS erzeugt
# /dev/bpf*-Geraeteknoten neu bei jedem Start (Default-Rechte root:wheel 0600) -- dieses Skript
# setzt sie auf die Gruppe "access_bpf", damit q9.exe --net bridge ohne sudo laufen kann.
# Idempotent, gefahrlos mehrfach ausfuehrbar.

for dev in /dev/bpf*; do
    [ -e "$dev" ] || continue
    chgrp access_bpf "$dev" 2>/dev/null
    chmod 660 "$dev" 2>/dev/null
done
