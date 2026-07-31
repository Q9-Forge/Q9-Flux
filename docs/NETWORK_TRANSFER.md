# Netzwerk-Dateitransfer im CB030-Teststand

Stand 2026-07-27: Das Testimage `local_images/OS9SYS.hda.smb209-test.hda`
enthaelt bereits die OS-9-SMB-Komponenten `samba` (Edition 209), `smbdrv`,
`smbmount` und `smbview`. Ausserdem sind `tftpd` und `tftpdc` vorhanden.

## Samba

`smbmount` ist der OS-9-SMB-Client. Der Aufruf erwartet einen UNC-Pfad:

```text
smbmount /dd/PROJECTS //192.168.200.1/SHARE -U=username -P=password
```

Die genaue Freigabe und die Zugangsdaten kommen vom Host-Samba-Dienst. Die
macOS-vmnet-Konfiguration muss verwendet werden, weil das aktuelle Q9-NAT
nur ARP/ICMP und keinen TCP-Transport bereitstellt:

```sh
sudo ./build/native/q9.exe emu.codex-samba.q9
```

Nach dem OS-9-Login kann der Mount-Aufruf getestet werden; `smbview` dient zur
Anzeige der verfuegbaren Shares.

## TFTP-Fallback

Der OS-9-TFTP-Server ist im Image enthalten, aber absichtlich nicht im
`SYS/startspf` aktiviert. Er kann nach dem Login ohne Image-Aenderung gestartet
werden:

```text
load -d /dd/CMDS tftpd tftpdc
tftpd /dd/PROJECTS <>>>/nil &
```

Vom Host aus liest/schreibt der TFTP-Client dann Dateien aus diesem Verzeichnis
ueber UDP Port 69. TFTP ist absichtlich nur als einfacher Transferweg gedacht;
es bietet keine Benutzeranmeldung oder Verzeichnisverwaltung.

## Reproduzierbarer Q9-Start

```sh
make native
sudo ./build/native/q9.exe emu.codex-samba.q9
```

Der Boot erreicht mit dem Testimage `8 devices online` und startet den
Netzterminal-Dienst auf Host-Port 2000. Der automatisierte Test konnte in der
Sandbox nicht bis zu Samba/TFTP durchlaufen: externe TCP-Verbindungen werden
hier mit `Operation not permitted` blockiert und `sudo` ist nicht verfuegbar.
Der Q9-Boot selbst und das Vorhandensein aller vier SMB- sowie beider
TFTP-Module wurden lokal verifiziert.
