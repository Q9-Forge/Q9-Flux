#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 1.20
# Owner:  Claudia
# Desc.:  Q9-Flux-Launcher/Config-Editor -- Grundstein (docs/Q9FLUX_EDITOR_de.md). Bewusst C99 ohne
#         TUI-Framework, s. src/q9_ansi.h Kopfkommentar (Vorgeschichte: gescheiterter Turbo-Vision-
#         Prototyp unter ../q9-launcher-prototype/).
#
# Call:   make test    (alle Selbsttests, kein Terminal noetig)
#         make demo    (interaktive Sichtpruefung -- baut UND fuehrt aus, echtes Terminal noetig)
#         make clean
#
# Edition History
#─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                            │ By
#─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
# 26-08-14│ 1.00 │ Erster Wurf -- ansi_selftest                                            │ Cld
# 26-08-16│ 1.10 │ screenbuf_selftest dazu (q9_screenbuf.h/.c, Bildschirmpuffer fuer den    │ Cld
#         │      │ modalen Config-Auswahl-Dialog, Q9FLUX_EDITOR_de.md Abschnitt 2)          │
# 26-08-16│ 1.20 │ widgets_selftest dazu (q9_widgets.h/.c, draw_frame -- ASCII-Rahmen mit   │ Cld
#         │      │ optionalem mittigen Titel)                                                │
#═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2
BUILD   := build

.PHONY: test demo clean

test: $(BUILD)/ansi_selftest $(BUILD)/screenbuf_selftest $(BUILD)/widgets_selftest
	$(BUILD)/ansi_selftest
	$(BUILD)/screenbuf_selftest
	$(BUILD)/widgets_selftest

demo: $(BUILD)/ansi_selftest
	$(BUILD)/ansi_selftest --demo

$(BUILD)/ansi_selftest: test/ansi_selftest.c src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/ansi_selftest.c src/q9_ansi.c -o $@

$(BUILD)/screenbuf_selftest: test/screenbuf_selftest.c src/q9_screenbuf.c src/q9_screenbuf.h src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/screenbuf_selftest.c src/q9_screenbuf.c src/q9_ansi.c -o $@

$(BUILD)/widgets_selftest: test/widgets_selftest.c src/q9_widgets.c src/q9_widgets.h src/q9_screenbuf.c src/q9_screenbuf.h src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/widgets_selftest.c src/q9_widgets.c src/q9_screenbuf.c src/q9_ansi.c -o $@

clean:
	rm -rf $(BUILD)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 1.20
#─────────────────────────────────────────────────────────────────────────────────────────────────
