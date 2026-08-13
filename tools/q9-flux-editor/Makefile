#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 1.00
# Owner:  Claudia
# Desc.:  Q9-Flux-Launcher/Config-Editor -- Grundstein (docs/Q9FLUX_EDITOR_de.md). Bewusst C99 ohne
#         TUI-Framework, s. src/q9_ansi.h Kopfkommentar (Vorgeschichte: gescheiterter Turbo-Vision-
#         Prototyp unter ../q9-launcher-prototype/).
#
# Call:   make test    (Selbsttest, Byte-Vergleich gegen den ANSI/VT100-Standard, kein Terminal noetig)
#         make demo    (interaktive Sichtpruefung -- baut UND fuehrt aus, echtes Terminal noetig)
#         make clean
#═════════════════════════════════════════════════════════════════════════════════════════════════
CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2
BUILD   := build

.PHONY: test demo clean

test: $(BUILD)/ansi_selftest
	$(BUILD)/ansi_selftest

demo: $(BUILD)/ansi_selftest
	$(BUILD)/ansi_selftest --demo

$(BUILD)/ansi_selftest: test/ansi_selftest.c src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/ansi_selftest.c src/q9_ansi.c -o $@

clean:
	rm -rf $(BUILD)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
