#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Usage:
  tools/prepare-sanyo-page.sh [options] <page.pdf> [more-pages.pdf ...]

Options:
  -o, --out <dir>          Output root directory (default: work/sanyo-pages)
  -r, --resolution <dpi>   Render resolution (default: 300)

Examples:
  tools/prepare-sanyo-page.sh /mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-026_ocr.pdf
  tools/prepare-sanyo-page.sh -o work/sanyo-pages -r 400 page-026_ocr.pdf

Requires WSL/Linux tools:
  poppler-utils: pdftotext, pdftoppm, pdfinfo
  imagemagick : magick or convert, optional but recommended for crops
EOF
  exit 2
}

out_root="${OUT_ROOT:-work/sanyo-pages}"
dpi="${DPI:-300}"
pdfs=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    -o|--out)
      [[ $# -ge 2 ]] || usage
      out_root="$2"
      shift 2
      ;;
    -r|--resolution)
      [[ $# -ge 2 ]] || usage
      dpi="$2"
      shift 2
      ;;
    -h|--help)
      usage
      ;;
    *)
      pdfs+=("$1")
      shift
      ;;
  esac
done

[[ ${#pdfs[@]} -gt 0 ]] || usage

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

to_wsl_path() {
  local path="$1"
  if [[ "$path" == ?:* ]] && command -v wslpath >/dev/null 2>&1; then
    wslpath -u "$path"
  else
    printf '%s\n' "$path"
  fi
}

image_tool() {
  if command -v magick >/dev/null 2>&1; then
    printf '%s\n' "magick"
  elif command -v convert >/dev/null 2>&1; then
    printf '%s\n' "convert"
  else
    printf '%s\n' ""
  fi
}

identify_tool() {
  if command -v magick >/dev/null 2>&1; then
    printf '%s\n' "magick identify"
  elif command -v identify >/dev/null 2>&1; then
    printf '%s\n' "identify"
  else
    printf '%s\n' ""
  fi
}

crop_image() {
  local tool="$1"
  local image="$2"
  local out="$3"
  local geometry="$4"
  if [[ "$tool" == "magick" ]]; then
    magick "$image" -crop "$geometry" +repage "$out"
  else
    convert "$image" -crop "$geometry" +repage "$out"
  fi
}

require_command pdftotext
require_command pdftoppm
require_command pdfinfo

mkdir -p "$out_root"
img_tool="$(image_tool)"
id_tool="$(identify_tool)"

for raw_pdf in "${pdfs[@]}"; do
  pdf="$(to_wsl_path "$raw_pdf")"
  if [[ ! -f "$pdf" ]]; then
    echo "Input PDF not found: $raw_pdf" >&2
    exit 1
  fi

  base="$(basename "$pdf")"
  stem="${base%.pdf}"
  stem="${stem%_ocr}"
  out_dir="$out_root/$stem"
  mkdir -p "$out_dir"

  echo "Preparing $pdf"
  echo "Output: $out_dir"

  pdfinfo "$pdf" > "$out_dir/pdfinfo.txt"
  pdftotext -layout "$pdf" "$out_dir/page.txt"
  pdftoppm -png -r "$dpi" -singlefile "$pdf" "$out_dir/page"

  image="$out_dir/page.png"
  if [[ -n "$img_tool" && -n "$id_tool" && -f "$image" ]]; then
    read -r width height < <($id_tool -format '%w %h\n' "$image")

    table_x=$((width * 66 / 100))
    table_w=$((width - table_x))
    left_w=$table_x
    header_h=$((height * 16 / 100))
    examples_y=$((height * 43 / 100))
    examples_h=$((height * 20 / 100))
    operation_y=$((height * 62 / 100))
    operation_h=$((height - operation_y))

    crop_image "$img_tool" "$image" "$out_dir/program-table.png" "${table_w}x${height}+${table_x}+0"
    crop_image "$img_tool" "$image" "$out_dir/main-text.png" "${left_w}x${examples_y}+0+0"
    crop_image "$img_tool" "$image" "$out_dir/examples.png" "${left_w}x${examples_h}+0+${examples_y}"
    crop_image "$img_tool" "$image" "$out_dir/operation-notes.png" "${left_w}x${operation_h}+0+${operation_y}"
    crop_image "$img_tool" "$image" "$out_dir/header.png" "${left_w}x${header_h}+0+0"
  else
    echo "ImageMagick not found; skipped crop images." >&2
  fi

  {
    echo "# HEADER"
    echo "# Program Title: TODO"
    echo "# Program No.  : TODO"
    echo "# DEG/RAD      : TODO"
    echo "# DPS          : TODO"
    echo "#"
    echo "# FORMULA"
    echo "# TODO"
    echo "#"
    echo "# EXAMPLES"
    echo "# TODO"
    echo "#"
    echo "# OPERATION"
    echo "# TODO"
    echo "#"
    echo "# NOTES"
    echo "# TODO"
    echo "#"
    echo "# DATA MEMORY"
    echo "# TODO"
    echo ""
    echo "/LOAD"
    echo ""
    echo "# Source Code"
    echo "# TODO: transcribe program table"
    echo ""
    echo "# RAW OCR TEXT"
    sed 's/^/# /' "$out_dir/page.txt"
  } > "$out_dir/draft.sce"

  cat > "$out_dir/README.md" <<EOF
# $stem

Prepared from:

\`\`\`text
$pdf
\`\`\`

Files:

- \`page.txt\`: layout-preserving text from \`pdftotext -layout\`
- \`page.png\`: rendered page at ${dpi} dpi
- \`program-table.png\`: right-side program table crop
- \`main-text.png\`: title/formula crop
- \`examples.png\`: examples crop
- \`operation-notes.png\`: operation and notes crop
- \`draft.sce\`: initial source template with raw OCR text

Next steps:

1. Read \`page.png\` and \`program-table.png\`.
2. Fill header, formula, examples, operation, notes, and data memory in \`draft.sce\`.
3. Replace the source-code TODO with textual key names.
4. Verify with:

\`\`\`bash
node santron-cli.js --scenario-file "$out_dir/draft.sce" --scenario "list"
\`\`\`
EOF

  echo "Done: $out_dir"
done
