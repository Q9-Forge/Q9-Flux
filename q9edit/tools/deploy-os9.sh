#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
Q9_ROOT="$(cd "$ROOT/.." && pwd)"
MWOS_ENV="/Volumes/SSD1TB/projects/MWOS/tools/macos/env/os9-toolchain.sh"
PRODUCT_IMAGE="$Q9_ROOT/local_images/OS9SYS.hda"
MODULE="${MODULE:-$ROOT/os9/CMDS/qeprobe}"

usage() {
    echo "Usage: $0 /absolute/path/to/development-image.hda [guest-directory]" >&2
    echo "Default guest directory: CMDS" >&2
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 2
fi

IMAGE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GUEST_DIR="${2:-CMDS}"

if [[ ! -f "$IMAGE" ]]; then
    echo "q9edit: image not found: $IMAGE" >&2
    exit 1
fi

if [[ "$IMAGE" == "$PRODUCT_IMAGE" ]]; then
    echo "q9edit: refusing to modify production image: $IMAGE" >&2
    exit 1
fi

if [[ ! -f "$MODULE" ]]; then
    echo "q9edit: module not found: $MODULE" >&2
    echo "q9edit: run 'make -C q9edit os9' first" >&2
    exit 1
fi

if pgrep -afil 'build/native/q9|q9.exe' 2>/dev/null | grep -F -- "$IMAGE" >/dev/null; then
    echo "q9edit: refusing to write image used by a running emulator: $IMAGE" >&2
    exit 1
fi

if [[ ! -f "$MWOS_ENV" ]]; then
    echo "q9edit: MWOS environment not found: $MWOS_ENV" >&2
    exit 1
fi

# shellcheck source=/dev/null
source "$MWOS_ENV"

if [[ ! -x "${MWOS_TOOLSHED_OS9:-}" ]]; then
    echo "q9edit: ToolShed os9 command is unavailable" >&2
    exit 1
fi

echo "q9edit: copying $(basename "$MODULE") to $IMAGE,$GUEST_DIR"
"$MWOS_TOOLSHED_OS9" copy -r "$MODULE" "$IMAGE,$GUEST_DIR"
"$MWOS_TOOLSHED_OS9" attr -q "$IMAGE,$GUEST_DIR/$(basename "$MODULE")" \
    -e -r -w -pe -pr
echo "q9edit: deployment complete"

