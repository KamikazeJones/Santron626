#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$ROOT_DIR/build.sh"
java -cp "$ROOT_DIR/out" dev.quassel.ga.puzzle.PuzzleCli "$@"
