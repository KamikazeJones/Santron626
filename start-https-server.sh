#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
server_script="$repo_root/https-server.py"
cert_file="$repo_root/certs/santron626-local.crt"
key_file="$repo_root/certs/santron626-local.key"

if [[ ! -f "$server_script" ]]; then
  echo "Server script not found: $server_script" >&2
  exit 1
fi

if [[ ! -f "$cert_file" ]]; then
  echo "Certificate not found: $cert_file" >&2
  exit 1
fi

if [[ ! -f "$key_file" ]]; then
  echo "Key not found: $key_file" >&2
  exit 1
fi

python_cmd=""
for candidate in python3 python py; do
  if command -v "$candidate" >/dev/null 2>&1; then
    python_cmd="$candidate"
    break
  fi
done

if [[ -z "$python_cmd" ]]; then
  echo "No Python interpreter found. Tried: python3, python, py" >&2
  exit 1
fi

cd "$repo_root"
echo "Starting HTTPS server from $repo_root"
exec "$python_cmd" "$server_script"
