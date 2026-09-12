# Boot smoke tests

## OS-9000 U4.9 XiBase9 graphics

Date: 2026-09-03

Start command:

```sh
bin/flux-x86 --config profiles/os9000-4.9.conf
```

The profile uses the local, Git-ignored `images/flux-x86.img` in QEMU snapshot
mode.  QMP reported the guest as running in 32-bit protected mode.  The initial
VGA console displayed the OS-9000 prompt and the instruction:

```text
To start XiBase please use:
xb <Enter>
```

Sending `xb` and Enter through QMP changed the display from 720x400 text mode
to a 1024x768 XiBase9 desktop.  Visible desktop elements included Start, Trash,
File Manager and Terminal Program.

This establishes the first end-to-end baseline for Q9-Flux-x86:

```text
local OS-9000 image -> flux-x86 profile -> QEMU i386 PC -> OS-9000 boot -> XiBase9
```

## OS-9000 U4.9 network reconnaissance

Date: 2026-09-03

The original U4.9 distribution includes native `ftp`, `telnet`, `dhcp`,
`ipstart`, and `ping` commands, plus the SPF/IP/TCP/UDP/NFS stack.  `ftp` is a
real interactive client with `get`, `put`, `mget`, and `mput`; it is the most
promising normal host/guest file-transfer path.

The initial live test used QEMU user networking and the former `pcnet` NIC
profile.  At the OS-9000 prompt, `ping 10.0.2.2` reached the command but failed
with `sendto: 007:030` / `ret=-1`.  Running `dhcp` then reported that `enet0`
has no hardware address and failed with `#000:225 (E_Param)`.  Thus networking
is not configured in the booted image yet; this is not evidence that the guest
commands are absent.

The U4.9 installation media contain `llne2000` and `spne2000`, but no PCnet
driver.  The reference profile now therefore uses QEMU's `ne2k_pci` emulation.
The matching runtime modules are `spne2000` and descriptor `spne0`.

### Transfer-media installation attempt

A FAT32 transfer image containing the required SPF/IP/NE2000 modules was
attached as a secondary IDE drive with `--transfer-disk`.  The guest could not
open `/pc1h` because `pcf` was not linked; `load pcf` then failed with
`#000:216 (E_PNNF) File not found`.  The XiBase image is thus a minimal boot
image: it lacks both the network stack and the PCF file manager required to
import that stack.

The next integration task is a narrowly scoped host-side OS-9000/x86 RBF file
injector.  It need only create directories and files in a copied test image,
initially to install PCF and the SPF/NE2000 modules.  Once those are present,
test DHCP, `ping 10.0.2.2`, then FTP to a host service on a non-privileged port
(for example `10.0.2.2:2121`).

### 2026-09-04: Native archive-installation attempt

The XiBase root already contains `mw86.tar`, the U4.9 resident archive which
contains `CMDS/BOOTOBJS/pcf` and the complete SPF/NE2000 set.  On a disposable
copy of the image, `tar xvf mw86.tar` completed successfully and created the
full `CMDS`, `SYS`, `TFTPBOOT`, and `ASSETS` trees.

This is **not** a safe installer for the XiBase image.  In the still-running
minimal boot environment, the extracted `load` command failed with `E_FNA`.
After reboot, the image reached a shell but the startup script could no longer
execute even `tmode`, `setime`, `link`, `list`, or `diskcache` (all `E_FNA`).
The U4.9 resident archive therefore overwrites system files with a set that is
incompatible with this XiBase boot volume.  The original image was never
modified; only the disposable copy was used and then stopped.

Consequently the host-side injector must be selective: first add only PCF and
the minimum compatible SPF modules, preserving all existing XiBase files and
boot metadata.  Do not install `mw86.tar` wholesale.

### 2026-09-04: PCF/FAT device-path diagnosis

On a fresh disposable copy, extracting only `CMDS/BOOTOBJS/pcf` from the
guest's existing `mw86.tar` and running `load -d CMDS/BOOTOBJS/pcf` succeeds.
This preserves the XiBase-supplied `load` program; replacing that program from
the archive is incompatible and must be avoided.

The expected secondary-IDE name `/pc1h` is not a device descriptor in this
boot image (`E_MNF`).  The existing floppy descriptor is `/d0`: without media
it returns `E_Read`, so the controller and descriptor are present.  A dedicated
1.44-MB FAT transfer image was then attached with `--floppy`; after PCF was
loaded, `/d0` returned `E_BTyp` (incompatible media).  This establishes that
the boot image configures `/d0` for RBF, not PCF.  The FAT image itself is not
the fault.

The next FAT task is therefore to build/link a PCF variant of the floppy
device descriptor (or generate a matching boot image with `DD_DEVICE_PCF`)
while retaining the current RBF `/d0` descriptor.  Once that descriptor is
available, the prepared FAT floppy contains the minimum SPF/IP/NE2000 files;
then install them selectively and run `dhcp spne0` followed by `ping
10.0.2.2`.

The archived XiBase configuration file confirms the exact missing build-time
choice: `[FLOPPY_SUPPORT]` has `PCF=TRUE`, but `DD_DEVICE_PCF=FALSE`.
Therefore the PCF manager may be present while `/d0` is deliberately emitted
only as an RBF descriptor. `bootgen` is available in the OS-9000 files, but it
only writes an already-built bootstrap; the configuration generator that turns
this INI choice into descriptor modules is not present in this installation.

### Descriptor reconstruction evidence

A QEMU physical-memory snapshot of a clean XiBase boot contains the live
`d0` descriptor at physical offset `0x246D58`. It is an OS-9000 device
descriptor module (`MT_DEVDESC`, type 15), 532 bytes long. Its relevant
in-module offsets are now measured rather than inferred:

| Offset | Current value | PCF variant |
| --- | --- | --- |
| `0x64` | file-manager name offset `0xCE` → `rbf` | same offset → `pcf` |
| `0x68` | driver name offset `0xC8` → `rb765` | unchanged |
| `0x5C` | `dd_lu_num`, floppy unit `0` (QEMU drive A) | `1` for QEMU drive B |
| `0x1EA` | RB765 private floppy-unit byte `0` | `1` for QEMU drive B |
| `0x60` | `dd_type` `1` (RBF) | `10` (x86 PCF) |
| `0xD2` | descriptor name `d0` | `p0` |

This gives a safe construction recipe for an additional `p0` module: clone
the live descriptor, make only these three semantic substitutions, then
recalculate its module CRC. It leaves the working RBF `d0` path untouched.
The first `p0` live-load test proved that PCF recognizes the synthesized
descriptor, but it returned `E_LockID`: RBF `d0` and PCF `p0` were trying to
own the same floppy unit.  The descriptor's measured `dd_lu_num` field at
`0x5C` therefore lets the next variant (`p1`) select QEMU's second floppy
drive.  The descriptor builder accepts `--name p1 --unit 1`; this keeps the
working RBF `d0` on drive A while PCF owns the FAT medium on drive B.

`p1` has been generated from QEMU's ELF core dump and independently checked:
it is 532 bytes, has `pcf` at `0xCE`, `p1` at `0xD2`, unit `1` at `0x5C`, and
the x86 PCF device type `10` at `0x60`, with CRC24 residual `0x800FE3`.  QEMU attaches the prepared
FAT image as floppy index 1.  The x86 RBF staging path is now operational:
the guest formats a native Little-Endian RBF floppy, and the host injector
overwrites a guest-created large regular-file template with the module data.
Keeping that file descriptor intact is required by the currently reconstructed
format; `F$Load` uses the module's own size and successfully loads `p1`.

The original synthesized descriptor had written `DT_PCF` to `0x6C`.  That is
`dd_class`, not the device type.  The OS-9000 `dd_com` definition confirms
`dd_lu_num` at `0x5C` and `dd_type` at `0x60`, both 16-bit.  The generic DPIO
header is not sufficient for this x86 build: the extracted native
`RB765/d1_3.d1` and `RB765/md1_3.md1` pair is the authoritative reference.
It changes the manager to `pcf`, sets `dd_type` to `10`, clears byte `0xB0`
and word `0xE8..0xEB`, and uses the same RB765 driver/options.

The extracted native x86 RB765 `md0_3.md0` and `md1_3.md1` descriptors also
establish that, in addition to the public 16-bit `dd_lu_num` at `0x5C`, RB765
mirrors the unit as a byte at `0x1EA`.  The descriptor builder now applies all
of these verified differences before calculating the CRC.

The prepared transfer medium was subsequently found to contain a valid FAT12
filesystem only at partition LBA 1, preceded by an MBR.  That is an acceptable
hard-disk layout but not a floppy layout for PCF, which reads its FAT boot
record at physical sector 0.  `tools/unpartition_fat_floppy.py` converts this
specific one-partition image into a 1.44-MB FAT12 superfloppy without changing
its files.  `tools/build_fat12_floppy.py` also makes an empty conventional
1.44-MB FAT12 image for isolated tests.

### 2026-09-05: Native x86 PCF descriptor verified

The extracted OS-9000/x86 RB765 descriptor directory contains the native PCF
pair `md0_3.md0` and `md1_3.md1`.  Comparing `d1_3.d1` with `md1_3.md1`
provided the exact conversion values used by `build_pcf_descriptor.py`: PCF
manager name, `dd_type = 10`, byte `0xB0 = 0`, word `0xE8..0xEB = 0`, and the
RB765 unit mirror at `0x1EA`.

Loading a descriptor module alone does **not** attach it.  The validated guest
sequence is:

```sh
load -d /d0/p1
iniz p1
dir /p1
```

`devs` then reports `p1  rb765  ...  pcf`.  The empty standard FAT12 image
opens successfully.  The prepared network FAT image also opens successfully
and lists its root commands (`dhcp`, `ftp`, `ipstart`, `ping`) and the complete
`/p1/spf` stack including `spne2000` and `spne0`.  This completes the PCF/FAT
mounting milestone; the next task is selective runtime loading of the network
modules, followed by DHCP and ping.

### 2026-09-07: NE2000/SPF packet-path diagnosis

With QEMU's ISA NE2000 at the native `spne0` I/O address `0x340`, the x86
`spne2000` driver performs real NE2000 register and data-port transactions;
the QEMU trace proves hardware initialization and transmit-buffer writes.
`ifconfig -a` reports a working `enet0` with MAC `52:54:00:12:34:56` and the
configured address `10.0.2.14/24`.  Consequently `dhcp enet0` reads the MAC
correctly and reports that the static address already exists.  `spne0` is only
the hardware descriptor and is not the argument expected by `dhcp`.

The remaining network failure is above the hardware driver: `ping 10.0.2.2`
creates no ARP request for the gateway.  QEMU's backend packet capture records
only the guest's gratuitous ARP announcements, while `netstat -i` keeps both
input and output packet counters at zero and ARP reports the gateway as
incomplete.  ISA IRQ 3, 5, and 10 show the same result.

This was independently reproduced through the VGA shell with
`dhcp enet0 -override -nofork -v -timeout 3 -tries 1`: it removes `10.0.2.14`,
adds `0.0.0.0`, and reports repeated `Sending DHCPDISCOVER to 255.255.255.255`.
During those reported sends, the QEMU packet capture remains unchanged and the
NE2000 trace records no further command-register value `0x24` (the transmit
trigger).  The startup gratuitous ARPs prove that the lower driver can transmit
when SPF initializes it, so the next task is to reconstruct the live SPF
device graph and `enet0` state transition after an address change.  It is no
longer an NE2000 port, IRQ, FAT, PCF, or DHCP-client issue.

The OS-9000 shell is on the VGA terminal, not on COM1.  For repeatable
headless experiments use `tools/qmp_type.py`, for example:

```
tools/qmp_type.py /tmp/flux-x86.qmp 'load -d /d0/p1' --enter
```

It injects keystrokes through QMP and avoids treating the serial-TCP endpoint
as an OS-9000 command console.

### 2026-09-07: `inetdb3` capacity defect in the XiBase init sequence

The XiBase profile's `SYS_PARAMS` currently runs `ipstart` and then creates
`inetdb3` with this allocation:

```
ndbmod create inetdb3 11 0 0 0 0 0 0 400 0 0 65 0
```

`ndbmod` defines allocation slot 9 as the host-interface database.  It is
zero in that command.  In a clean snapshot, `ndbmod interface del enet0`
followed by an interface add fails with `Error #000:248 (E_Full) Media Full`.
That is a direct, independent explanation for why dynamic DHCP/interface
updates cannot create the state needed to pass packets to the lower driver.

The supplied SPF reference startup script instead creates the dynamic database
*before* `ipstart` and reserves interface space:

```
ndbmod create inetdb3 11 400 0 160 0 0 0 100 0 400 65 256
ipstart
```

The durable fix is to regenerate the boot image's init module with that order
and allocation (and then perform the normal `ndbmod interface add enet0 ...
binding /spne0/enet` setup).  The proprietary disk image is intentionally not
patched in place; the next verification is an image rebuilt from the corrected
init configuration, followed by DHCP and gateway-ping capture.

#### Live runtime workaround verified

On a disposable writable XiBase copy, the existing zero-sized interface can be
removed and rebound without rebuilding the image:

```text
ndbmod interface del enet0
ndbmod interface add enet0 binding /spne0/enet
dhcp enet0 -override -nofork -v -timeout 3 -tries 1
```

This successfully completed DHCP in QEMU user networking and assigned
`10.0.2.15/24`, gateway `10.0.2.2`, and DNS `10.0.2.3`.  With the DHCP client
left running in the background, ICMP replies were verified from `10.0.2.2`,
`10.0.2.3`, and `8.8.8.8`.  A host-side TCP listener on port 2323 also
received the payload `HELLO` from the guest's `telnet 10.0.2.2 2323` session.
The earlier negative ping result was a test-procedure error: interrupting the
foreground DHCP command removed the address before the ping ran.  The result
proves that the runtime rebind restores the complete NE2000/SPF packet path;
the remaining work is to make the corrected `inetdb3` allocation persistent
in the boot image.

The same sequence is automated for reboot tests by
`tools/qmp_network_bootstrap.py`. Start QEMU with a QMP Unix socket, wait for
the normal console boot, and run:

### 2026-09-08: DNS initialization order

The BLS networking startup example documents that `ndbmod resolve ...` belongs
between creation of `inetdb3` and `ipstart`; it is not merely a runtime setting
for an already-running stack.  Our persistent patch currently starts `ipstart`
from `Init` and adds the interface/DHCP commands from `/SYS/startup`, so adding
`ndbmod resolve` there does not yet make hostname lookup work.  The guest can
reach `192.168.2.3` and public IP addresses, but `ping example.com` still
reports `unknown host`.  The next focused experiment is therefore a disposable
`Init` variant with the resolver entry before `ipstart`, followed by a clean
boot test; no unverified DNS change is kept in the committed image patch.

```sh
python3 tools/qmp_network_bootstrap.py /tmp/flux-x86.qmp
```

The helper is intended to run with the launcher's default user network:
`192.168.0.0/16`, DHCP start `192.168.77.15`, and guest-visible host/gateway
`192.168.123.250`. It does not modify the guest image. The older `10.0.2.x`
values above document the original baseline test and remain valid when those
network options are selected explicitly.

#### FTP file transfer verified

The guest FTP client was tested against a controlled host FTP service on the
custom `192.168.77.0/24` user network.  The URL form with embedded credentials
was used:

```text
ftp -p -v -o /dd/ftpout ftp://anonymous:foo@192.168.77.2:2121/hello.txt
```

The server observed `USER`, `PASS`, `SIZE`, `EPSV`, and `RETR hello.txt`; the
guest then read `/dd/ftpout` and returned `Q9 FTP transfer works`.  The test
server did not implement the optional `MDTM` response, which produced only a
timestamp parsing warning after the successful download.  FTP is therefore a
working practical transfer path for the current emulator setup.

`bootgen` is available in the guest.  On a disposable writable copy,
`bootgen -v /hc1` identifies `/hc1/sysboot` and `/hc1/firstboot` as the two
current bootstrap files.  `sysboot` is a bootstrap container rather than a
stand-alone module, so `ident /hc1/sysboot` correctly reports a bad module ID.
The remaining build step is to generate a corrected `Init` module and pass it
to `bootgen`; it must not be attempted by overwriting arbitrary bytes in the
vendor image.

#### Bootgen target-path limitation

An additional disposable-image test rebuilt a bootstrap successfully on the
format-enabled descriptor `/hc1fmt` and produced `/hc1fmt/sysboot`.  The IDE
bootstrap nevertheless continues to use `/h0/sysboot`.  Copying the generated
file over that path from the running system fails with `E_Share` because the
active bootstrap has the original `/h0/sysboot` open non-shareably.  Therefore
the guest-side `bootgen` route is useful for validating modules and format
parameters, but it cannot replace the active XiBase bootstrap in-place.  A
working persistent patch must modify the XiBase `sysboot` container offline (or
boot from a separately prepared volume) before starting the guest.

The offline path is implemented by `tools/patch_x86_sysboot.py`.  It
decompresses the existing OS9Z payload, replaces a module, rebuilds the zlib
stream and both OS9Z headers, then updates the x86-RBF descriptor and parity
on a new image copy.  Example:

```sh
python3 tools/patch_x86_sysboot.py \
  /path/to/os9000-xibase.img /tmp/os9000-initpatched.img \
  /tmp/init-live-patched.bin
```

This was verified on a disposable copy: the resulting image passed the BIOS
compressed-bootfile step and reached the normal OS-9000 shell prompt.

The same tool accepts `--startup` to replace `/SYS/startup` in the image while
preserving its allocated x86-RBF data blocks:

```sh
python3 tools/patch_x86_sysboot.py \
  /path/to/os9000-xibase.img /tmp/os9000-network.img \
  /tmp/init-live-patched.bin --startup /tmp/startup-network.txt
```

The resulting test image was rebooted without `qmp_network_bootstrap.py`.
`ifconfig -a` showed `enet0` up with `192.168.77.15/16` and gateway
`192.168.123.250`; the Python helper is therefore no longer needed for a
patched image.

### XiBase volume-format correction

The on-disk file descriptor for `CMDS` starts with `FD B0 B0 FD`, rather than
a classic RBF file descriptor. Its segment fields also do not map directly to
an RBF directory under either 256-byte or 512-byte LSN scaling. The XiBase
image therefore uses a XiBase-specific filesystem layer behind the `XD00BT`
BIOS bootstrap; its use of the RBF file manager at runtime does not make its
host format classic RBF. Do not use 68k RBF writers on this image.

### GDB RAM-injection probe

`flux-x86 --gdb <port>` starts QEMU paused with its system GDB stub. LLDB
successfully wrote the 532-byte generated `p0` module to address `0x03F00000`
and immediately read back its valid OS-9000 header (`FC 4A 02 00 ...`). This
proves that a non-persistent descriptor-transfer path is available without
altering the XiBase image. The remaining work is OS-9000 module-directory
registration; merely placing a valid module in RAM does not make it visible to
`F$Link`/`iniz`.
