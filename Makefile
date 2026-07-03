#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 1.00
# Owner:  AF
# Desc.:  Q9 Build-System. Targets: native (PC, gcc/w64devkit), wasm (Browser, emcc), test, clean.
#         Toolchain-Setup siehe docs/TOOLCHAIN.md.
#
# Call:   make native | make wasm | make test | make clean
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-02│ 1.00 │ Initiale Version: native + wasm + test                                  │ CF
# 26-07-03│ 1.10 │ 1.3: device.c + dev_term.c, Test 03                                     │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

CC      = gcc
EMCC    = emcc
CFLAGS  = -std=c99 -Wall -Wextra -O2

BUILD   = build
KSRC    = src/kernel/kernel.c src/kernel/syscall.c src/kernel/device.c src/kernel/dev_term.c \
          src/kernel/dev_nil.c src/kernel/name.c
HDRS    = src/hal/q9_hal.h src/kernel/kernel.h src/kernel/syscall.h src/kernel/device.h \
          src/kernel/name.h

#───────────────────────────────────────────────────────────────────────────────────────────────
# native: PC-Build (Windows, w64devkit)
#───────────────────────────────────────────────────────────────────────────────────────────────
native: $(BUILD)/native/q9.exe

$(BUILD)/native/q9.exe: $(KSRC) src/hal/native/hal_native.c $(HDRS)
	@mkdir -p $(BUILD)/native
	$(CC) $(CFLAGS) $(KSRC) src/hal/native/hal_native.c -o $@

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
	python test/01_test_boot.py
	python test/02_test_syscalls.py
	python test/03_test_devices.py

clean:
	rm -rf $(BUILD)

.PHONY: native wasm test clean

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
