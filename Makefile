#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 3.40
# Owner:  AF
# Desc.:  Q9-Flux Build-System (68030-Emulator fuer echtes OS-9/68k).
#         Targets: host (Wirtssystem-Build, gcc/w64devkit oder clang/gcc), test, clean.
#
# Call:   make host | make test | make test-cf-sector | make clean   (make native = Alias)
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-02│ 1.00 │ Initiale Version: native + wasm + test                                  │ CF
# ... (fruehere Historie siehe docs/PROJECT_VISION_ARCHIV.md und Git-Historie)             │
# 26-07-31│ 3.00 │ Eigener Mini-Kernel + wasm3 + Browser-Frontend nach Q9RESUME-Kernel      │ CF
#         │      │ ausgelagert (unbenutzt seit 26-07-04) -- native baut jetzt ausschliesslich │
#         │      │ den OS-9/68k-Emulator, kein wasm-Target mehr                 │
# 26-08-06│ 3.10 │ Nativer Windows-Build (Winsock2 statt BSD-Sockets in m68krt.c/           │ AF
#         │      │ videobridge.c, s. src/kernel/q9_sockcompat.h) -- -lws2_32 unter Windows   │
# 26-08-06│ 3.20 │ PLATFORM_DIR: build/native/ war fuer alle drei OS gleich benannt -- baut  │ AF
#         │      │ man denselben Checkout auf mehreren Plattformen, ueberschrieben sich die  │
#         │      │ Artefakte. Jetzt build/windows|macos|linux/ (Kommando bleibt "make native")│
# 26-08-07│ 3.30 │ 5.14: slirp-Backend (--net slirp) -- Windows gegen third_party/slirp/      │ AF
#         │      │ windows/ (vendorte libslirp+glib2, kein Paketmanager noetig), macOS/Linux  │
#         │      │ gegen System-libslirp per pkg-config; DLLs werden nach dem Link neben      │
#         │      │ q9.exe kopiert                                                             │
# 26-08-11│ 3.40 │ 6.6: Geraete (mc6845/clut/framebuf/quicc/videobridge/slirp_net/vmnet_net/  │ AF
#         │      │ bpf_net) von src/kernel/ nach src/devices/<name>/ verschoben (je ein         │
#         │      │ Unterordner pro Modul, Mehrarchitektur-Vorbereitung); q9board.c/m68krt.c/   │
#         │      │ q9boardrun.c/devreg.c/boardcfg.c bleiben bewusst in src/kernel/ (Bus/CPU-    │
#         │      │ Wrapper/Framework, keine eigenstaendigen Geraete)                            │
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
PYTHON  = $(shell command -v python3 2>/dev/null || command -v python)

BUILD   = build
HDRS    = src/hal/q9_hal.h

# PLATFORM/PLATFORM_DIR: baut man denselben Checkout (z.B. ueber eine Netzwerkfreigabe)
# abwechselnd unter Windows/macOS/Linux, ueberschreiben sich die Objektdateien/Binaries, wenn
# alle drei denselben Pfad build/native/* benutzen. Jede Plattform bekommt daher ihren eigenen
# Unterordner (build/windows/, build/macos/, build/linux/); das Kommando bleibt ueberall
# "make native" ("baue fuer die Maschine, auf der ich gerade bin" -- die uebliche Bedeutung von
# "native" in Build-Systemen, in Abgrenzung zu Cross-Compile-Targets).
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
else
    UNAME_S  = $(shell uname -s 2>/dev/null)
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
    else
        PLATFORM = linux
    endif
endif
PLATFORM_DIR = $(PLATFORM)

# Host-HAL nach Wirtssystem waehlen: Windows (w64devkit setzt $OS=Windows_NT) = conio,
# alles andere (macOS/Linux) = POSIX/termios.
# "Host" statt "native" (2026-08-12): wir bauen fuer mindestens drei Wirtssysteme UND
# mindestens drei Zielarchitekturen -- "nativ" laesst offen, welches von beiden gemeint ist.
# Host = die Maschine, auf der der Emulator laeuft; Target = die Architektur, die er emuliert.
# Windows-Build: unter Windows brauchen die Netz-Terminals (m68krt.c) und die Video-Bridge (videobridge.c)
# Winsock2 statt BSD-Sockets (s. src/kernel/q9_sockcompat.h) -- -lws2_32 fuer WSAStartup/socket/...
ifeq ($(PLATFORM),windows)
    HOST_HAL_SRC = src/hal/windows/hal_windows.c
    HOST_EXTRA_LIBS = -lws2_32 -lwinmm
else
    HOST_HAL_SRC = src/hal/posix/hal_posix.c
endif

# 5.1: eingebettete Musashi-68000-Emulation (third_party/musashi, Entscheidung E12). Musashi hat
# einen Zweistufen-Build: m68kmake (selbst ein kleines Host-Tool) liest m68k_in.c (518
# handgeschriebene Opcode-Primitive) und generiert daraus m68kops.c/.h (1967 Opcode-Handler) --
# reine Build-Artefakte, landen unter $(MUSASHI_GEN) und werden nicht versioniert. M68KRT_SRC ist
# Q9-eigener Code (volle CFLAGS); MUSASHI_SRC ist unveraendert vendorter Fremdcode + generierter
# Code, uebersetzt mit eigenen, laxeren Flags.
M68KRT_SRC   = src/kernel/m68krt.c
M68KRT_HDR   = src/kernel/m68krt.h
MUSASHI_DIR  = third_party/musashi
MUSASHI_GEN  = $(BUILD)/$(PLATFORM_DIR)/musashi_gen
MUSASHI_MAKE = $(BUILD)/$(PLATFORM_DIR)/m68kmake
MUSASHI_CFLAGS = -std=c99 -O2 -I$(MUSASHI_DIR) -I$(MUSASHI_GEN)
# m68kcpu.c bindet m68kfpu.c bereits selbst per #include ein (Musashi-eigenes Muster, s.
# third_party/musashi/m68kcpu.c Zeile 51) -- m68kfpu.c darf deshalb NICHT separat uebersetzt
# werden, sonst doppelte Symbole (m68040_fpu_op0/op1) beim Linken.
MUSASHI_OBJS = $(BUILD)/$(PLATFORM_DIR)/musashi_m68kcpu.o $(BUILD)/$(PLATFORM_DIR)/musashi_softfloat.o \
               $(BUILD)/$(PLATFORM_DIR)/musashi_m68kops.o

$(MUSASHI_MAKE): $(MUSASHI_DIR)/m68kmake.c
	@mkdir -p $(BUILD)/$(PLATFORM_DIR)
	$(CC) -std=c99 -O2 -o $@ $<

$(MUSASHI_GEN)/m68kops.c $(MUSASHI_GEN)/m68kops.h: $(MUSASHI_MAKE) $(MUSASHI_DIR)/m68k_in.c
	@mkdir -p $(MUSASHI_GEN)
	$(MUSASHI_MAKE) $(MUSASHI_GEN)/ $(MUSASHI_DIR)/m68k_in.c

$(BUILD)/$(PLATFORM_DIR)/musashi_m68kcpu.o: $(MUSASHI_DIR)/m68kcpu.c $(MUSASHI_DIR)/m68kfpu.c \
                                   $(MUSASHI_DIR)/m68kmmu.h $(MUSASHI_DIR)/m68kcpu.h $(MUSASHI_GEN)/m68kops.h
	@mkdir -p $(BUILD)/$(PLATFORM_DIR)
	$(CC) $(MUSASHI_CFLAGS) -c $< -o $@

$(BUILD)/$(PLATFORM_DIR)/musashi_softfloat.o: $(MUSASHI_DIR)/softfloat/softfloat.c
	@mkdir -p $(BUILD)/$(PLATFORM_DIR)
	$(CC) $(MUSASHI_CFLAGS) -c $< -o $@

$(BUILD)/$(PLATFORM_DIR)/musashi_m68kops.o: $(MUSASHI_GEN)/m68kops.c
	@mkdir -p $(BUILD)/$(PLATFORM_DIR)
	$(CC) $(MUSASHI_CFLAGS) -c $< -o $@

# 5.2a: Board-Speicherlogik (RAM/ROM/Remap, docs/BOARD.md) -- Q9-eigener Code, volle CFLAGS
# wie M68KRT_SRC.
BOARD_SRC = src/kernel/q9board.c src/kernel/q9boardrun.c src/kernel/devreg.c src/kernel/boardcfg.c \
            src/devices/quicc/quicc.c src/devices/mc6845/mc6845.c src/devices/framebuf/framebuf.c \
            src/devices/clut/clut.c src/devices/videobridge/videobridge.c
BOARD_HDR = src/kernel/q9board.h src/kernel/q9boardrun.h src/kernel/devreg.h src/kernel/boardcfg.h \
            src/devices/quicc/quicc.h src/devices/mc6845/mc6845.h src/devices/framebuf/framebuf.h \
            src/devices/clut/clut.h src/devices/videobridge/videobridge.h

# 5.12: vmnet-Ethernet-Backend (--net vmnet), nur macOS: vmnet.framework + Dispatch/Blocks.
# 5.13: bridge-Ethernet-Backend (--net bridge:<ifname>), nur macOS: BPF (/dev/bpf*), kein Framework
# noetig (reines POSIX/ioctl). Auf anderen Plattformen bleiben beide Defines ungesetzt und die
# jeweilige --net-Option meldet sich sauber ab.
ifeq ($(shell uname -s 2>/dev/null),Darwin)
    BOARD_NET_SRC   = src/devices/net/vmnet_net.c src/devices/net/bpf_net.c
    BOARD_NET_HDR   = src/devices/net/vmnet_net.h src/devices/net/bpf_net.h
    BOARD_NET_FLAGS = -DQ9_HAVE_VMNET -DQ9_HAVE_BPF
    BOARD_NET_LIBS  = -framework vmnet
endif

# 5.14: slirp-Backend (--net slirp), PLATTFORMUEBERGREIFEND (anders als vmnet/bridge oben) -- unter
# Windows gegen die vendorten Dateien in third_party/slirp/windows/ (s. dortige Q9_VENDOR.md: kein
# Paketmanager noetig, w64devkit allein reicht), unter macOS/Linux gegen ein System-libslirp per
# pkg-config (brew install libslirp / apt install libslirp-dev). Fehlt beides, bleibt Q9_HAVE_SLIRP
# ungesetzt und "--net slirp" meldet sich beim Start sauber ab (s. quicc.c).
SLIRP_SRC = src/devices/net/slirp_net.c
SLIRP_HDR = src/devices/net/slirp_net.h
ifeq ($(PLATFORM),windows)
    ifneq ($(wildcard third_party/slirp/windows/include/slirp/libslirp.h),)
        SLIRP_VENDOR   = third_party/slirp/windows
        SLIRP_FLAGS    = -DQ9_HAVE_SLIRP -I$(SLIRP_VENDOR)/include \
                         -I$(SLIRP_VENDOR)/include/glib-2.0 -I$(SLIRP_VENDOR)/lib/glib-2.0/include
        SLIRP_LIBS     = -L$(SLIRP_VENDOR)/lib -lslirp -lglib-2.0
        SLIRP_RUNTIME_DLLS = $(wildcard $(SLIRP_VENDOR)/bin/*.dll)
    endif
else
    ifeq ($(shell pkg-config --exists slirp 2>/dev/null && echo yes),yes)
        # 26-08-10: Homebrews libslirp.pc liefert nur "-I<includedir>/slirp" (fuer #include
        # <libslirp.h>), unser slirp_net.c schreibt aber #include <slirp/libslirp.h> -- deshalb
        # zusaetzlich den PARENT-Include-Pfad (<includedir> selbst) mitgeben, damit beide
        # Schreibweisen funktionieren, ohne slirp_net.c anzufassen (Debian/Fedora-libslirp-Pakete
        # liefern ueblicherweise direkt "-I<includedir>" und brauchen diesen Zusatz nicht, schadet
        # dort aber auch nicht).
        SLIRP_FLAGS = -DQ9_HAVE_SLIRP $(shell pkg-config --cflags slirp) \
                      -I$(shell pkg-config --variable=includedir slirp)
        SLIRP_LIBS  = $(shell pkg-config --libs slirp)
    endif
endif

#───────────────────────────────────────────────────────────────────────────────────────────────
# host tools: portable C utilities (can later share their image-format core with Q9)
#───────────────────────────────────────────────────────────────────────────────────────────────
q9fat: $(BUILD)/tools/q9fat

$(BUILD)/tools/q9fat: tools/q9fat.c
	@mkdir -p $(BUILD)/tools
	$(CC) $(CFLAGS) $< -o $@

#───────────────────────────────────────────────────────────────────────────────────────────────
# host: Build fuer das Wirtssystem, auf dem gerade gebaut wird (Windows w64devkit oder
# macOS/Linux, HAL wird automatisch gewaehlt) -- landet unter build/windows|macos|linux/.
# "native" bleibt als stiller Alias erhalten, damit bestehende Skripte und Gewohnheiten
# weiterlaufen.
#───────────────────────────────────────────────────────────────────────────────────────────────
host native: $(BUILD)/$(PLATFORM_DIR)/q9.exe
	@echo "-> $(BUILD)/$(PLATFORM_DIR)/q9.exe"

$(BUILD)/$(PLATFORM_DIR)/q9.exe: $(M68KRT_SRC) $(M68KRT_HDR) \
                        $(BOARD_SRC) $(BOARD_HDR) $(BOARD_NET_SRC) $(BOARD_NET_HDR) \
                        $(SLIRP_SRC) $(SLIRP_HDR) \
                        $(HOST_HAL_SRC) $(HDRS) $(MUSASHI_OBJS)
	@mkdir -p $(BUILD)/$(PLATFORM_DIR)
	$(CC) $(CFLAGS) -DQ9_HAVE_M68K $(BOARD_NET_FLAGS) $(SLIRP_FLAGS) -I$(MUSASHI_DIR) \
	    $(M68KRT_SRC) $(BOARD_SRC) $(BOARD_NET_SRC) $(SLIRP_SRC) $(HOST_HAL_SRC) \
	    $(MUSASHI_OBJS) $(BOARD_NET_LIBS) $(SLIRP_LIBS) $(HOST_EXTRA_LIBS) -o $@
ifneq ($(SLIRP_RUNTIME_DLLS),)
	@cp $(SLIRP_RUNTIME_DLLS) $(BUILD)/$(PLATFORM_DIR)/
endif

#───────────────────────────────────────────────────────────────────────────────────────────────
# test / clean
#───────────────────────────────────────────────────────────────────────────────────────────────
test: test-cf-sector

# 5.19b: dateisystem-unabhaengiger Sektor-Roundtrip-Test der CF-Emulation (q9board.c) -- reines
# ATA-PIO-Protokoll gegen q9_cf_attach/q9_devtype_cf, ohne 68k-CPU/OS-9/RBF/PCF-Treiber.
test-cf-sector:
	@mkdir -p $(BUILD)/$(PLATFORM_DIR)
	$(CC) $(CFLAGS) test/07_test_cf_sector512.c test/07_hal_stub.c \
	    src/kernel/q9board.c src/kernel/devreg.c -o $(BUILD)/$(PLATFORM_DIR)/test_cf_sector512
	$(BUILD)/$(PLATFORM_DIR)/test_cf_sector512

clean:
	rm -rf $(BUILD)

.PHONY: host native q9fat test test-cf-sector clean

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 3.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
