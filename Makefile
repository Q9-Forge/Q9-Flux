#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 2.30
# Owner:  AF
# Desc.:  Q9 Build-System. Targets: native (PC, gcc/w64devkit oder macOS/Linux clang/gcc),
#         wasm (Browser, emcc), test, clean. Toolchain-Setup siehe docs/TOOLCHAIN.md.
#
# Call:   make native | make wasm | make test | make clean
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-02│ 1.00 │ Initiale Version: native + wasm + test                                  │ CF
# 26-07-03│ 1.10 │ 1.3: device.c + dev_term.c, Test 03                                     │ CF
# 26-07-03│ 1.20 │ 1.10: POSIX-HAL (macOS/Linux), native-Target waehlt HAL per OS,          │ CF
#         │      │ PYTHON-Erkennung (python3 vs. python) fuer test-Target                  │
# 26-07-03│ 1.30 │ 2.1: module.c/.h (Modul-Header + CRC32)                                 │ CF
# 26-07-03│ 1.40 │ 3.1: dev_d0.c (Roh-Block-Device), Test 04                               │ CF
# 26-07-04│ 1.50 │ 3.2: vfs.c/.h (VFS-Pfad-Routing), Test 05                               │ CF
# 26-07-04│ 1.60 │ 3.3: fat16.c/.h (FAT16 lesend), Test 06                                 │ CF
# 26-07-04│ 1.70 │ 3.4: test-Target loescht q9disk.img vor dem Lauf (sonst kann ein         │ CF
#         │      │ FAT16-Image aus einem frueheren "make test" — mit z.B. NEUDIR aus dem   │
#         │      │ 06-Selbsttest — die Tests 01-05 verwirren, bevor 06 es neu aufbaut)      │
# 26-07-04│ 1.80 │ 3.6: wasm-Target kopiert web/worker.js mit (Kernel laeuft jetzt im       │ CF
#         │      │ Worker, OPFS-Blockgeraet)                                               │
# 26-07-04│ 1.90 │ 4.1: proc.c/.h (Prozess-Descriptor-Tabelle + Round-Robin-Scheduler)      │ CF
# 26-07-04│ 2.00 │ 4.6: wasmrt.c/.h (wasm3-Wrapper) + vendorte third_party/wasm3/ nur im     │ CF
#         │      │ native-Target; -DQ9_HAVE_WASM3 aktiviert den Selbsttest-Zweig in kernel.c │
# 26-07-04│ 2.10 │ 4.9: WASM3_CFLAGS bindet src/kernel/config.h ein und setzt               │ CF
#         │      │ d_m3FixedHeap=Q9_SYSTEM_MEM_BYTES (Fixed-Heap statt Host-malloc in wasm3)  │
# 26-07-04│ 2.20 │ 5.1: m68krt.c/.h (Musashi-Wrapper) + vendorte third_party/musashi/ nur im  │ CF
#         │      │ native-Target; m68kmake generiert m68kops.c/.h zur Bauzeit (Zweistufen-    │
#         │      │ Build); -DQ9_HAVE_M68K aktiviert den Selbsttest-Zweig in kernel.c          │
# 26-07-04│ 2.30 │ 5.2a: cb030.c/.h (Board-Speicherlogik RAM/ROM/Remap), nur native-Target    │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

CC      = gcc
EMCC    = emcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
PYTHON  = $(shell command -v python3 2>/dev/null || command -v python)

BUILD   = build
KSRC    = src/kernel/kernel.c src/kernel/syscall.c src/kernel/device.c src/kernel/dev_term.c \
          src/kernel/dev_nil.c src/kernel/dev_d0.c src/kernel/name.c src/kernel/module.c \
          src/kernel/vfs.c src/kernel/fat16.c src/kernel/proc.c
HDRS    = src/hal/q9_hal.h src/kernel/kernel.h src/kernel/syscall.h src/kernel/device.h \
          src/kernel/name.h src/kernel/module.h src/kernel/vfs.h src/kernel/fat16.h \
          src/kernel/proc.h

# native-HAL nach Betriebssystem waehlen: Windows (w64devkit setzt $OS=Windows_NT) = conio,
# alles andere (macOS/Linux) = POSIX/termios.
ifeq ($(OS),Windows_NT)
    NATIVE_HAL_SRC = src/hal/native/hal_native.c
else
    NATIVE_HAL_SRC = src/hal/posix/hal_posix.c
endif

# 4.6: eingebettete wasm3-Runtime, NUR im nativen Build (im Browser laeuft Q9 selbst schon als
# WASM, s. third_party/wasm3/README.md). WASMRT_SRC ist Q9-eigener Code (volle CFLAGS, bleibt
# warnungsfrei); WASM3_SRC ist unveraendert vendorter Fremdcode und wird bewusst mit eigenen,
# laxeren Flags uebersetzt (58 -Wall/-Wextra-Warnungen im Original, die wir nicht pflegen).
# 4.9: -include src/kernel/config.h + -Dd_m3FixedHeap=Q9_SYSTEM_MEM_BYTES schalten wasm3 auf einen
# statischen Fixed-Heap um (third_party/wasm3/m3_config.h: d_m3FixedHeap ist #ifndef-geschuetzt,
# das Command-Line-Define gewinnt) -- kein einziges Byte am vendorten Fremdcode selbst geaendert.
WASMRT_SRC   = src/kernel/wasmrt.c src/kernel/wasmproc.c
WASMRT_HDR   = src/kernel/wasmrt.h src/kernel/wasmproc.h
Q9_CONFIG_HDR = src/kernel/config.h
WASM3_DIR    = third_party/wasm3
WASM3_SRC    = $(WASM3_DIR)/m3_bind.c $(WASM3_DIR)/m3_code.c $(WASM3_DIR)/m3_compile.c \
               $(WASM3_DIR)/m3_core.c $(WASM3_DIR)/m3_env.c $(WASM3_DIR)/m3_exec.c \
               $(WASM3_DIR)/m3_function.c $(WASM3_DIR)/m3_info.c $(WASM3_DIR)/m3_module.c \
               $(WASM3_DIR)/m3_parse.c
WASM3_CFLAGS = -std=c99 -O2 -I$(WASM3_DIR) -include $(Q9_CONFIG_HDR) -Dd_m3FixedHeap=Q9_SYSTEM_MEM_BYTES
WASM3_OBJS   = $(patsubst $(WASM3_DIR)/%.c,$(BUILD)/native/wasm3_%.o,$(WASM3_SRC))

$(BUILD)/native/wasm3_%.o: $(WASM3_DIR)/%.c
	@mkdir -p $(BUILD)/native
	$(CC) $(WASM3_CFLAGS) -c $< -o $@

# 5.1: eingebettete Musashi-68000-Emulation (third_party/musashi, Entscheidung E12), NUR im
# nativen Build (analog zu wasm3 in 4.6). Musashi hat einen Zweistufen-Build: m68kmake (selbst ein
# kleines Host-Tool) liest m68k_in.c (518 handgeschriebene Opcode-Primitive) und generiert daraus
# m68kops.c/.h (1967 Opcode-Handler) -- reine Build-Artefakte (wie WASM3_OBJS), landen unter
# $(MUSASHI_GEN) und werden nicht versioniert. M68KRT_SRC ist Q9-eigener Code (volle CFLAGS);
# MUSASHI_SRC ist unveraendert vendorter Fremdcode + generierter Code, uebersetzt mit eigenen,
# laxeren Flags (wie WASM3_CFLAGS).
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

# 5.2a: CB030-Board-Speicherlogik (RAM/ROM/Remap, docs/CB030.md), NUR im nativen Build (reine
# Musashi-Bootstrap-Validierung, s. cb030.h) -- Q9-eigener Code, volle CFLAGS wie M68KRT_SRC.
CB030_SRC = src/kernel/cb030.c src/kernel/cb030run.c
CB030_HDR = src/kernel/cb030.h src/kernel/cb030run.h

#───────────────────────────────────────────────────────────────────────────────────────────────
# native: PC-Build (Windows w64devkit oder macOS/Linux, HAL wird automatisch gewaehlt)
#───────────────────────────────────────────────────────────────────────────────────────────────
native: $(BUILD)/native/q9.exe

$(BUILD)/native/q9.exe: $(KSRC) $(WASMRT_SRC) $(WASMRT_HDR) $(M68KRT_SRC) $(M68KRT_HDR) \
                        $(CB030_SRC) $(CB030_HDR) \
                        $(NATIVE_HAL_SRC) $(HDRS) $(WASM3_OBJS) $(MUSASHI_OBJS)
	@mkdir -p $(BUILD)/native
	$(CC) $(CFLAGS) -DQ9_HAVE_WASM3 -DQ9_HAVE_M68K -I$(WASM3_DIR) -I$(MUSASHI_DIR) \
	    $(KSRC) $(WASMRT_SRC) $(M68KRT_SRC) $(CB030_SRC) $(NATIVE_HAL_SRC) $(WASM3_OBJS) $(MUSASHI_OBJS) -o $@

#───────────────────────────────────────────────────────────────────────────────────────────────
# wasm: Browser-Build (Emscripten); kopiert das Frontend mit nach build/wasm/
#───────────────────────────────────────────────────────────────────────────────────────────────
wasm: $(BUILD)/wasm/q9.js

$(BUILD)/wasm/q9.js: $(KSRC) src/hal/wasm/hal_wasm.c $(HDRS) web/index.html web/worker.js
	@mkdir -p $(BUILD)/wasm
	$(EMCC) $(CFLAGS) $(KSRC) src/hal/wasm/hal_wasm.c -o $@ \
	    -sEXPORTED_FUNCTIONS=_q9_kernel_init,_q9_kernel_step
	cp web/index.html web/worker.js $(BUILD)/wasm/

#───────────────────────────────────────────────────────────────────────────────────────────────
# test / clean
#───────────────────────────────────────────────────────────────────────────────────────────────
test: native
	@rm -f q9disk.img cb030_cf_test.img
	$(PYTHON) test/01_test_boot.py
	$(PYTHON) test/02_test_syscalls.py
	$(PYTHON) test/03_test_devices.py
	$(PYTHON) test/04_test_blkdev.py
	$(PYTHON) test/05_test_vfs.py
	$(PYTHON) test/06_test_fat16.py

clean:
	rm -rf $(BUILD)

.PHONY: native wasm test clean

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 2.20
#─────────────────────────────────────────────────────────────────────────────────────────────────
