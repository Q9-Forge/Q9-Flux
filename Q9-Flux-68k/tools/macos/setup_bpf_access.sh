#!/bin/sh
#════════════════════════════════════════════════════════════════════════════════════════════════
# setup_bpf_access.sh -- 5.13: Einmaliges Setup fuer den Q9-Bridge-Netzwerkmodus (--net bridge).
#
# BPF-Geraete (/dev/bpf*) gehoeren auf macOS standardmaessig root:wheel mit Modus 0600 -- ohne
# Anpassung braucht q9.exe deshalb root/sudo, um sie zu oeffnen. Dieses Skript richtet das
# klassische "ChmodBPF"-Muster ein (dasselbe, das z.B. Wireshark installiert):
#
#   1. Legt die Gruppe "access_bpf" an (falls noch nicht vorhanden).
#   2. Nimmt den aufrufenden Benutzer in diese Gruppe auf.
#   3. Installiert ein LaunchDaemon, das bei jedem Boot (und bei Aenderungen unter /dev) die
#      Gruppenrechte der /dev/bpf*-Geraete auf "access_bpf" setzt (die Geraeteknoten werden bei
#      jedem Start neu angelegt, die Rechte muessen deshalb bei jedem Boot neu gesetzt werden).
#   4. Wendet die Rechte sofort einmal an, damit KEIN Neustart noetig ist.
#
# Danach kann q9.exe --net bridge:<ifname> OHNE sudo laufen. Muss selbst mit sudo aufgerufen
# werden: sudo tools/macos/setup_bpf_access.sh
#
# WICHTIG: Braucht eine dedizierte physische Ethernet-Schnittstelle (WLAN laesst sich auf den
# meisten Access Points nicht bridgen) -- `ifconfig -l` zeigt alle Namen, `--net bridge:en5` waehlt
# eine davon aus.
#
# Deinstallation: sudo launchctl bootout system/com.q9.chmod-bpf 2>/dev/null
#                 sudo rm /Library/LaunchDaemons/com.q9.chmod-bpf.plist
#                 sudo rm -rf "/Library/Application Support/Q9"
#                 sudo dseditgroup -o delete access_bpf   # optional, falls sonst niemand sie nutzt
#════════════════════════════════════════════════════════════════════════════════════════════════
set -e

if [ "$(id -u)" != "0" ]; then
    echo "Bitte mit sudo aufrufen: sudo $0" >&2
    exit 1
fi

TARGET_USER="${SUDO_USER:-$(stat -f%Su /dev/console)}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUPPORT_DIR="/Library/Application Support/Q9"
DAEMON_SCRIPT="$SUPPORT_DIR/chmod_bpf.sh"
PLIST="/Library/LaunchDaemons/com.q9.chmod-bpf.plist"
LABEL="com.q9.chmod-bpf"

echo "==> Benutzer: $TARGET_USER"

echo "==> Gruppe 'access_bpf' anlegen (falls noetig)..."
dseditgroup -o read access_bpf >/dev/null 2>&1 || dseditgroup -o create access_bpf

echo "==> '$TARGET_USER' zu 'access_bpf' hinzufuegen..."
dseditgroup -o edit -a "$TARGET_USER" -t user access_bpf

echo "==> Boot-Skript nach '$SUPPORT_DIR' installieren..."
mkdir -p "$SUPPORT_DIR"
cp "$SCRIPT_DIR/chmod_bpf.sh" "$DAEMON_SCRIPT"
chmod 755 "$DAEMON_SCRIPT"
chown root:wheel "$DAEMON_SCRIPT"

echo "==> LaunchDaemon '$LABEL' installieren..."
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>$LABEL</string>
    <key>ProgramArguments</key>
    <array>
        <string>$DAEMON_SCRIPT</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>WatchPaths</key>
    <array>
        <string>/dev</string>
    </array>
</dict>
</plist>
EOF
chown root:wheel "$PLIST"
chmod 644 "$PLIST"

echo "==> LaunchDaemon laden..."
launchctl bootout system "$PLIST" 2>/dev/null || true
launchctl bootstrap system "$PLIST" 2>/dev/null || launchctl load -w "$PLIST"

echo "==> Rechte sofort anwenden (kein Neustart noetig)..."
"$DAEMON_SCRIPT"

echo
echo "Fertig. '$TARGET_USER' muss sich einmal ab-/anmelden (Gruppenmitgliedschaft), danach laeuft"
echo "q9.exe --net bridge:<ifname> ohne sudo. Verfuegbare Schnittstellen zeigt: ifconfig -l"
