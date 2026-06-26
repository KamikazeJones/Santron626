#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$ROOT_DIR/javadoc"
TMP_DIR="$ROOT_DIR/javadoc.tmp"
DOCS_DIR="$ROOT_DIR/docs"
START_PAGE="$ROOT_DIR/start.html"
OVERVIEW_FILE="$ROOT_DIR/.javadoc-overview.tmp.html"

if ! command -v pandoc >/dev/null 2>&1; then
  echo "Fehler: pandoc ist nicht installiert oder nicht im PATH." >&2
  exit 1
fi

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

pandoc \
  --from=gfm \
  --to=html5 \
  --standalone \
  --no-highlight \
  --toc \
  --toc-depth=2 \
  --css docs/start.css \
  --metadata pagetitle="Java GA playground" \
  "$DOCS_DIR/navigation-start.md" \
  "$ROOT_DIR/README.md" \
  -o "$START_PAGE"

pandoc \
  --from=gfm \
  --to=html5 \
  --no-highlight \
  "$DOCS_DIR/navigation-javadoc.md" \
  "$ROOT_DIR/README.md" \
  | {
      printf '%s\n' '<html><body>'
      cat
      printf '%s\n' '</body></html>'
    } > "$OVERVIEW_FILE"

javadoc \
  -overview "$OVERVIEW_FILE" \
  -d "$TMP_DIR" \
  $(find "$ROOT_DIR/src/main/java" -name '*.java')

rm -f "$OVERVIEW_FILE"
rm -rf "$OUT_DIR"
mv "$TMP_DIR" "$OUT_DIR"
