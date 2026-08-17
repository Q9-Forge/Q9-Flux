#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Makefile                                                                        Ver. 1.80
# Owner:  Claudia
# Desc.:  Q9-Flux-Launcher/Config-Editor -- Grundstein (docs/Q9FLUX_EDITOR_de.md). Bewusst C99 ohne
#         TUI-Framework, s. src/q9_ansi.h Kopfkommentar (Vorgeschichte: gescheiterter Turbo-Vision-
#         Prototyp unter ../q9-launcher-prototype/).
#
# Call:   make test              (alle Selbsttests, kein Terminal noetig)
#         make demo              (rohe ANSI-Positionsprobe -- baut UND fuehrt aus, echtes Terminal noetig)
#         make demo-integration  (alle Bausteine zusammen: Rahmen+Liste+Tastatur+Resize)
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
# 26-08-16│ 1.30 │ procspawn_selftest dazu (q9_procspawn.h/.c, fork/exec/wait -- Grundlage │ Cld
#         │      │ fuer den "Start"-Button, Andreas' Entscheidung: Kindprozess statt exec()) │
# 26-08-16│ 1.40 │ input_selftest dazu (q9_input.h/.c, Tastatur-Rohmodus + Pfeiltasten/    │ Cld
#         │      │ Enter/Escape/Strg-C-Erkennung -- letzter Grundbaustein aus dem Plan)     │
# 26-08-16│ 1.50 │ listview_selftest dazu (q9_listview.h/.c, scrollbare Listenansicht    │ Cld
#         │      │ inkl. Scrollbalken -- Andreas' Frage nach einem "Bereich Scrollbar")     │
# 26-08-17│ 1.60 │ demo-integration dazu (demo/integration_demo.c) -- alle sechs Bausteine │ Cld
#         │      │ zusammen in einem echten Bildschirm, Andreas wollte sich das ansehen     │
# 26-08-17│ 1.70 │ filelist_selftest dazu (q9_filelist.h/.c, Verzeichnis-Scan -- Grundlage │ Cld
#         │      │ fuer den kommenden Datei-Auswahl-Dialog)                                 │
# 26-08-17│ 1.80 │ filedialog_selftest dazu (q9_filedialog.h/.c, modaler Datei-Auswahl-    │ Cld
#         │      │ Dialog -- task #20, letzter fehlender UI-Baustein aus dem Plan)          │
#═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2
BUILD   := build

.PHONY: test demo demo-integration clean

test: $(BUILD)/ansi_selftest $(BUILD)/screenbuf_selftest $(BUILD)/widgets_selftest $(BUILD)/procspawn_selftest $(BUILD)/input_selftest $(BUILD)/listview_selftest $(BUILD)/filelist_selftest $(BUILD)/filedialog_selftest
	$(BUILD)/ansi_selftest
	$(BUILD)/screenbuf_selftest
	$(BUILD)/widgets_selftest
	$(BUILD)/procspawn_selftest
	$(BUILD)/input_selftest
	$(BUILD)/listview_selftest
	$(BUILD)/filelist_selftest
	$(BUILD)/filedialog_selftest

demo: $(BUILD)/ansi_selftest
	$(BUILD)/ansi_selftest --demo

demo-integration: $(BUILD)/integration_demo
	$(BUILD)/integration_demo

$(BUILD)/integration_demo: demo/integration_demo.c src/q9_ansi.c src/q9_screenbuf.c src/q9_widgets.c src/q9_listview.c src/q9_input.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) demo/integration_demo.c src/q9_ansi.c src/q9_screenbuf.c src/q9_widgets.c src/q9_listview.c src/q9_input.c -o $@

$(BUILD)/ansi_selftest: test/ansi_selftest.c src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/ansi_selftest.c src/q9_ansi.c -o $@

$(BUILD)/screenbuf_selftest: test/screenbuf_selftest.c src/q9_screenbuf.c src/q9_screenbuf.h src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/screenbuf_selftest.c src/q9_screenbuf.c src/q9_ansi.c -o $@

$(BUILD)/widgets_selftest: test/widgets_selftest.c src/q9_widgets.c src/q9_widgets.h src/q9_screenbuf.c src/q9_screenbuf.h src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/widgets_selftest.c src/q9_widgets.c src/q9_screenbuf.c src/q9_ansi.c -o $@

$(BUILD)/procspawn_selftest: test/procspawn_selftest.c src/q9_procspawn.c src/q9_procspawn.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/procspawn_selftest.c src/q9_procspawn.c -o $@

$(BUILD)/input_selftest: test/input_selftest.c src/q9_input.c src/q9_input.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/input_selftest.c src/q9_input.c -o $@

$(BUILD)/listview_selftest: test/listview_selftest.c src/q9_listview.c src/q9_listview.h src/q9_screenbuf.c src/q9_screenbuf.h src/q9_ansi.c src/q9_ansi.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/listview_selftest.c src/q9_listview.c src/q9_screenbuf.c src/q9_ansi.c -o $@

$(BUILD)/filelist_selftest: test/filelist_selftest.c src/q9_filelist.c src/q9_filelist.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/filelist_selftest.c src/q9_filelist.c -o $@

$(BUILD)/filedialog_selftest: test/filedialog_selftest.c src/q9_filedialog.c src/q9_filedialog.h \
                               src/q9_filelist.c src/q9_listview.c src/q9_screenbuf.c \
                               src/q9_input.c src/q9_ansi.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) test/filedialog_selftest.c src/q9_filedialog.c src/q9_filelist.c \
	    src/q9_listview.c src/q9_screenbuf.c src/q9_input.c src/q9_ansi.c \
	    -o $@

clean:
	rm -rf $(BUILD)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Makefile                                                                            Ver. 1.70
#─────────────────────────────────────────────────────────────────────────────────────────────────
