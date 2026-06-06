#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: ./commit-build.sh \"Commit message\""
  exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root"

stamp_file="index.html"
now="$(date "+%Y-%m-%d %H:%M")"
current_stamp="$(grep -Eo "Build( [0-9]+)? [0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}" "$stamp_file" | head -n 1 || true)"

if [ -z "$current_stamp" ]; then
  echo "Build stamp not found in $stamp_file"
  exit 1
fi

build_number=0
if [[ "$current_stamp" =~ ^Build[[:space:]]+([0-9]+)[[:space:]]+[0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]]+[0-9]{2}:[0-9]{2}$ ]]; then
  build_number="${BASH_REMATCH[1]}"
fi

next_build=$((build_number + 1))
next_stamp="Build $next_build $now"
tmp_file="$(mktemp)"
trap 'rm -f "$tmp_file"' EXIT

awk -v old="$current_stamp" -v new="$next_stamp" '
  index($0, old) && !done {
    sub(old, new)
    done = 1
  }
  { print }
' "$stamp_file" > "$tmp_file"
mv "$tmp_file" "$stamp_file"
trap - EXIT

echo "Updated build stamp: $next_stamp"
git add .
git commit -m "$*"
git push
