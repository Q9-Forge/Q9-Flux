#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MWOS_ENV="/Volumes/SSD1TB/projects/MWOS/tools/macos/env/os9-toolchain.sh"

if [[ ! -f "$MWOS_ENV" ]]; then
    echo "q9edit: MWOS environment not found: $MWOS_ENV" >&2
    exit 1
fi

# shellcheck source=/dev/null
source "$MWOS_ENV"

cd "$ROOT/os9"
exec os9make "$@"

