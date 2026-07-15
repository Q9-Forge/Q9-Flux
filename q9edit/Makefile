.PHONY: help host os9

HOST_CC ?= cc
HOST_CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror
HOST_BUILD = build/host

help:
	@echo "q9edit (OS-9 command: qe)"
	@echo "make host - strict ANSI C89 host build"
	@echo "make os9  - build the current xcc toolchain probe"
	@echo "deploy with: q9edit/tools/deploy-os9.sh DEVELOPMENT.hda"

host:
	@mkdir -p $(HOST_BUILD)
	$(HOST_CC) $(HOST_CFLAGS) -o $(HOST_BUILD)/qe src/editor/qe.c

os9:
	./tools/build-os9.sh
