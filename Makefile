#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 2.00
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
WASMRT_SRC   = src/kernel/wasmrt.c
WASMRT_HDR   = src/kernel/wasmrt.h
WASM3_DIR    = third_party/wasm3
WASM3_SRC    = $(WASM3_DIR)/m3_bind.c $(WASM3_DIR)/m3_code.c $(WASM3_DIR)/m3_compile.c \
               $(WASM3_DIR)/m3_core.c $(WASM3_DIR)/m3_env.c $(WASM3_DIR)/m3_exec.c \
               $(WASM3_DIR)/m3_function.c $(WASM3_DIR)/m3_info.c $(WASM3_DIR)/m3_module.c \
               $(WASM3_DIR)/m3_parse.c
WASM3_CFLAGS = -std=c99 -O2 -I$(WASM3_DIR)
WASM3_OBJS   = $(patsubst $(WASM3_DIR)/%.c,$(BUILD)/native/wasm3_%.o,$(WASM3_SRC))

$(BUILD)/native/wasm3_%.o: $(WASM3_DIR)/%.c
	@mkdir -p $(BUILD)/native
	$(CC) $(WASM3_CFLAGS) -c $< -o $@

#───────────────────────────────────────────────────────────────────────────────────────────────
# native: PC-Build (Windows w64devkit oder macOS/Linux, HAL wird automatisch gewaehlt)
#───────────────────────────────────────────────────────────────────────────────────────────────
native: $(BUILD)/native/q9.exe

$(BUILD)/native/q9.exe: $(KSRC) $(WASMRT_SRC) $(WASMRT_HDR) $(NATIVE_HAL_SRC) $(HDRS) $(WASM3_OBJS)
	@mkdir -p $(BUILD)/native
	$(CC) $(CFLAGS) -DQ9_HAVE_WASM3 -I$(WASM3_DIR) \
	    $(KSRC) $(WASMRT_SRC) $(NATIVE_HAL_SRC) $(WASM3_OBJS) -o $@

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
	@rm -f q9disk.img
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
# EOF Makefile                                                                            Ver. 2.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
