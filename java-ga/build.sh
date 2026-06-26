#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$ROOT_DIR/out"
TMP_DIR="$ROOT_DIR/out.tmp"
LOCK_DIR="$ROOT_DIR/.build-lock"
STATE_FILE="$OUT_DIR/.build-state"
NEXT_STATE_FILE="$TMP_DIR/.build-state"

while ! mkdir "$LOCK_DIR" 2>/dev/null; do
  sleep 0.1
done

cleanup() {
  rm -rf "$TMP_DIR"
  rmdir "$LOCK_DIR"
}
trap cleanup EXIT

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"
find "$ROOT_DIR/src/main/java" -name '*.java' -print0 \
  | sort -z \
  | xargs -0 stat -c '%n %Y' > "$NEXT_STATE_FILE"
if [ -f "$STATE_FILE" ] && cmp -s "$NEXT_STATE_FILE" "$STATE_FILE"; then
  exit 0
fi
javac -d "$TMP_DIR" $(find "$ROOT_DIR/src/main/java" -name '*.java')
rm -rf "$OUT_DIR"
mv "$TMP_DIR" "$OUT_DIR"
