#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 1.00
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
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

CC      = gcc
EMCC    = emcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
PYTHON  = $(shell command -v python3 2>/dev/null || command -v python)

BUILD   = build
KSRC    = src/kernel/kernel.c src/kernel/syscall.c src/kernel/device.c src/kernel/dev_term.c \
          src/kernel/dev_nil.c src/kernel/dev_d0.c src/kernel/name.c src/kernel/module.c
HDRS    = src/hal/q9_hal.h src/kernel/kernel.h src/kernel/syscall.h src/kernel/device.h \
          src/kernel/name.h src/kernel/module.h

# native-HAL nach Betriebssystem waehlen: Windows (w64devkit setzt $OS=Windows_NT) = conio,
# alles andere (macOS/Linux) = POSIX/termios.
ifeq ($(OS),Windows_NT)
    NATIVE_HAL_SRC = src/hal/native/hal_native.c
else
    NATIVE_HAL_SRC = src/hal/posix/hal_posix.c
endif

#───────────────────────────────────────────────────────────────────────────────────────────────
# native: PC-Build (Windows w64devkit oder macOS/Linux, HAL wird automatisch gewaehlt)
#───────────────────────────────────────────────────────────────────────────────────────────────
native: $(BUILD)/native/q9.exe

$(BUILD)/native/q9.exe: $(KSRC) $(NATIVE_HAL_SRC) $(HDRS)
	@mkdir -p $(BUILD)/native
	$(CC) $(CFLAGS) $(KSRC) $(NATIVE_HAL_SRC) -o $@

#───────────────────────────────────────────────────────────────────────────────────────────────
# wasm: Browser-Build (Emscripten); kopiert das Frontend mit nach build/wasm/
#───────────────────────────────────────────────────────────────────────────────────────────────
wasm: $(BUILD)/wasm/q9.js

$(BUILD)/wasm/q9.js: $(KSRC) src/hal/wasm/hal_wasm.c $(HDRS) web/index.html
	@mkdir -p $(BUILD)/wasm
	$(EMCC) $(CFLAGS) $(KSRC) src/hal/wasm/hal_wasm.c -o $@ \
	    -sEXPORTED_FUNCTIONS=_q9_kernel_init,_q9_kernel_step
	cp web/index.html $(BUILD)/wasm/

#───────────────────────────────────────────────────────────────────────────────────────────────
# test / clean
#───────────────────────────────────────────────────────────────────────────────────────────────
test: native
	$(PYTHON) test/01_test_boot.py
	$(PYTHON) test/02_test_syscalls.py
	$(PYTHON) test/03_test_devices.py
	$(PYTHON) test/04_test_blkdev.py

clean:
	rm -rf $(BUILD)

.PHONY: native wasm test clean

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
