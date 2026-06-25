#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$ROOT_DIR/out"
TMP_DIR="$ROOT_DIR/out.tmp"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"
javac -d "$TMP_DIR" $(find "$ROOT_DIR/src/main/java" -name '*.java')
rm -rf "$OUT_DIR"
mv "$TMP_DIR" "$OUT_DIR"
