#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 3.00
# Owner:  AF
# Desc.:  Q9-Flux Build-System (CB030/68030-Emulator fuer echtes Microware-OS-9).
#         Targets: native (PC, gcc/w64devkit oder macOS/Linux clang/gcc), test, clean.
#
# Call:   make native | make test | make test-cf-sector | make clean
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-02│ 1.00 │ Initiale Version: native + wasm + test                                  │ CF
# ... (fruehere Historie siehe docs/PROJECT_VISION_ARCHIV.md und Git-Historie)             │
# 26-07-31│ 3.00 │ Eigener Mini-Kernel + wasm3 + Browser-Frontend nach Q9RESUME-Kernel      │ CF
#         │      │ ausgelagert (unbenutzt seit 26-07-04) -- native baut jetzt ausschliesslich │
#         │      │ den CB030/Microware-OS-9-Emulator, kein wasm-Target mehr                 │
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
PYTHON  = $(shell command -v python3 2>/dev/null || command -v python)

BUILD   = build
HDRS    = src/hal/q9_hal.h

# native-HAL nach Betriebssystem waehlen: Windows (w64devkit setzt $OS=Windows_NT) = conio,
# alles andere (macOS/Linux) = POSIX/termios.
ifeq ($(OS),Windows_NT)
    NATIVE_HAL_SRC = src/hal/native/hal_native.c
else
    NATIVE_HAL_SRC = src/hal/posix/hal_posix.c
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
MUSASHI_GEN  = $(BUILD)/native/musashi_gen
MUSASHI_MAKE = $(BUILD)/native/m68kmake
MUSASHI_CFLAGS = -std=c99 -O2 -I$(MUSASHI_DIR) -I$(MUSASHI_GEN)
# m68kcpu.c bindet m68kfpu.c bereits selbst per #include ein (Musashi-eigenes Muster, s.
# third_party/musashi/m68kcpu.c Zeile 51) -- m68kfpu.c darf deshalb NICHT separat uebersetzt
# werden, sonst doppelte Symbole (m68040_fpu_op0/op1) beim Linken.
MUSASHI_OBJS = $(BUILD)/native/musashi_m68kcpu.o $(BUILD)/native/musashi_softfloat.o \
               $(BUILD)/native/musashi_m68kops.o

$(MUSASHI_MAKE): $(MUSASHI_DIR)/m68kmake.c
	@mkdir -p $(BUILD)/native
	$(CC) -std=c99 -O2 -o $@ $<

$(MUSASHI_GEN)/m68kops.c $(MUSASHI_GEN)/m68kops.h: $(MUSASHI_MAKE) $(MUSASHI_DIR)/m68k_in.c
	@mkdir -p $(MUSASHI_GEN)
	$(MUSASHI_MAKE) $(MUSASHI_GEN)/ $(MUSASHI_DIR)/m68k_in.c

$(BUILD)/native/musashi_m68kcpu.o: $(MUSASHI_DIR)/m68kcpu.c $(MUSASHI_DIR)/m68kfpu.c \
                                   $(MUSASHI_DIR)/m68kmmu.h $(MUSASHI_DIR)/m68kcpu.h $(MUSASHI_GEN)/m68kops.h
	@mkdir -p $(BUILD)/native
	$(CC) $(MUSASHI_CFLAGS) -c $< -o $@

$(BUILD)/native/musashi_softfloat.o: $(MUSASHI_DIR)/softfloat/softfloat.c
	@mkdir -p $(BUILD)/native
	$(CC) $(MUSASHI_CFLAGS) -c $< -o $@

$(BUILD)/native/musashi_m68kops.o: $(MUSASHI_GEN)/m68kops.c
	@mkdir -p $(BUILD)/native
	$(CC) $(MUSASHI_CFLAGS) -c $< -o $@

# 5.2a: CB030-Board-Speicherlogik (RAM/ROM/Remap, docs/CB030.md) -- Q9-eigener Code, volle CFLAGS
# wie M68KRT_SRC.
CB030_SRC = src/kernel/cb030.c src/kernel/cb030run.c src/kernel/quicc.c src/kernel/devreg.c \
            src/kernel/boardcfg.c
CB030_HDR = src/kernel/cb030.h src/kernel/cb030run.h src/kernel/quicc.h src/kernel/devreg.h \
            src/kernel/boardcfg.h

# 5.12: vmnet-Ethernet-Backend (--net vmnet), nur macOS: vmnet.framework + Dispatch/Blocks.
# 5.13: bridge-Ethernet-Backend (--net bridge:<ifname>), nur macOS: BPF (/dev/bpf*), kein Framework
# noetig (reines POSIX/ioctl). Auf anderen Plattformen bleiben beide Defines ungesetzt und die
# jeweilige --net-Option meldet sich sauber ab.
ifeq ($(shell uname -s 2>/dev/null),Darwin)
    CB030_NET_SRC   = src/kernel/vmnet_net.c src/kernel/bpf_net.c
    CB030_NET_HDR   = src/kernel/vmnet_net.h src/kernel/bpf_net.h
    CB030_NET_FLAGS = -DQ9_HAVE_VMNET -DQ9_HAVE_BPF
    CB030_NET_LIBS  = -framework vmnet
endif

#───────────────────────────────────────────────────────────────────────────────────────────────
# host tools: portable C utilities (can later share their image-format core with Q9)
#───────────────────────────────────────────────────────────────────────────────────────────────
q9fat: $(BUILD)/tools/q9fat

$(BUILD)/tools/q9fat: tools/q9fat.c
	@mkdir -p $(BUILD)/tools
	$(CC) $(CFLAGS) $< -o $@

#───────────────────────────────────────────────────────────────────────────────────────────────
# native: PC-Build (Windows w64devkit oder macOS/Linux, HAL wird automatisch gewaehlt)
#───────────────────────────────────────────────────────────────────────────────────────────────
native: $(BUILD)/native/q9.exe

$(BUILD)/native/q9.exe: $(M68KRT_SRC) $(M68KRT_HDR) \
                        $(CB030_SRC) $(CB030_HDR) $(CB030_NET_SRC) $(CB030_NET_HDR) \
                        $(NATIVE_HAL_SRC) $(HDRS) $(MUSASHI_OBJS)
	@mkdir -p $(BUILD)/native
	$(CC) $(CFLAGS) -DQ9_HAVE_M68K $(CB030_NET_FLAGS) -I$(MUSASHI_DIR) \
	    $(M68KRT_SRC) $(CB030_SRC) $(CB030_NET_SRC) $(NATIVE_HAL_SRC) \
	    $(MUSASHI_OBJS) $(CB030_NET_LIBS) -o $@

#───────────────────────────────────────────────────────────────────────────────────────────────
# test / clean
#───────────────────────────────────────────────────────────────────────────────────────────────
test: test-cf-sector

# 5.19b: dateisystem-unabhaengiger Sektor-Roundtrip-Test der CF-Emulation (cb030.c) -- reines
# ATA-PIO-Protokoll gegen q9_cf_attach/q9_devtype_cf, ohne 68k-CPU/OS-9/RBF/PCF-Treiber.
test-cf-sector:
	@mkdir -p $(BUILD)/native
	$(CC) $(CFLAGS) test/07_test_cf_sector512.c test/07_hal_stub.c \
	    src/kernel/cb030.c src/kernel/devreg.c -o $(BUILD)/native/test_cf_sector512
	$(BUILD)/native/test_cf_sector512

clean:
	rm -rf $(BUILD)

.PHONY: native q9fat test test-cf-sector clean

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 3.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
