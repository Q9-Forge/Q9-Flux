.PHONY: help host test-host os9

HOST_CC ?= cc
HOST_CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror -DQE_HAVE_SIGWINCH
HOST_BUILD = build/host

help:
	@echo "q9edit (OS-9 command: qe)"
	@echo "make host - strict ANSI C89 host build"
	@echo "make test-host - build and run the host terminal smoke test"
	@echo "make os9  - build the current xcc toolchain probe"
	@echo "deploy with: q9edit/tools/deploy-os9.sh DEVELOPMENT.hda"

host:
	@mkdir -p $(HOST_BUILD)
	$(HOST_CC) $(HOST_CFLAGS) -Isrc/platform -o $(HOST_BUILD)/qe \
		src/editor/qe.c src/platform/qe_platform_posix.c \
		src/platform/qe_format.c

test-host: host
	./test/host_smoke.exp

os9:
	./tools/build-os9.sh
