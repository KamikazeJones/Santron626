#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPORT_DIR="${ROOT_DIR}/test-results"
REPORT_FILE="${REPORT_DIR}/index.html"
LOG_FILE="${REPORT_DIR}/results.log"
GUI_URL="${SANTRON_GUI_URL:-https://192.168.2.122:8765}"
STARTED_LOCAL_SERVER=0
LOCAL_SERVER_PID=""
GUI_SERVER_READY=0

mkdir -p "${REPORT_DIR}"
: > "${LOG_FILE}"

tests=(
  "build|node build.js"
  "unit|node test.js"
  "coverage|node tools/core-key-coverage.mjs"
  "gui-smoke|node tools/playwright-smoke.mjs ${GUI_URL}"
)

rows=()
overall_pass=1
PASS_ICON="✓"
FAIL_ICON="🛑"

status_label() {
  local status="$1"
  if [[ "${status}" == "PASS" ]]; then
    printf '%s %s' "${PASS_ICON}" "${status}"
  else
    printf '%s %s' "${FAIL_ICON}" "${status}"
  fi
}

run_test() {
  local name="$1"
  local cmd="$2"
  local tmp_log
  tmp_log="$(mktemp)"
  {
    echo "=== ${name} ==="
    echo "\$ ${cmd}"
  } >> "${LOG_FILE}"

  if bash -lc "cd \"${ROOT_DIR}\" && ${cmd}" >"${tmp_log}" 2>&1; then
    status="PASS"
  else
    status="FAIL"
    overall_pass=0
  fi

  cat "${tmp_log}" >> "${LOG_FILE}"
  {
    echo
    echo
  } >> "${LOG_FILE}"

  local escaped_output
  escaped_output="$(python3 - "${tmp_log}" <<'PY'
import html, pathlib, sys
path = pathlib.Path(sys.argv[1])
print(html.escape(path.read_text(encoding="utf-8", errors="replace")))
PY
)"

  rows+=(
    "<tr class=\"${status,,}\"><td>${name}</td><td class=\"status\">$(status_label "${status}")</td><td><pre>${escaped_output}</pre></td></tr>"
  )

  rm -f "${tmp_log}"
}

ensure_gui_server() {
  if curl --max-time 3 -k -I "${GUI_URL}" >/dev/null 2>&1; then
    return 0
  fi

  if [[ -n "${LOCAL_SERVER_PID}" ]]; then
    return 0
  fi

  if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not available; cannot start local GUI server" >> "${LOG_FILE}"
    return 1
  fi

  python3 "${ROOT_DIR}/https-server.py" >>"${LOG_FILE}" 2>&1 &
  LOCAL_SERVER_PID="$!"
  STARTED_LOCAL_SERVER=1

  for _ in {1..30}; do
    if curl --max-time 3 -k -I "https://localhost:8765" >/dev/null 2>&1; then
      GUI_URL="https://localhost:8765"
      return 0
    fi
    sleep 1
  done

  echo "local GUI server did not start in time" >> "${LOG_FILE}"
  return 1
}

cleanup() {
  if [[ -n "${LOCAL_SERVER_PID}" ]]; then
    kill "${LOCAL_SERVER_PID}" >/dev/null 2>&1 || true
    wait "${LOCAL_SERVER_PID}" >/dev/null 2>&1 || true
  fi
}

trap cleanup EXIT

if ! ensure_gui_server; then
  overall_pass=0
else
  GUI_SERVER_READY=1
fi

for item in "${tests[@]}"; do
  IFS='|' read -r name cmd <<<"${item}"
  if [[ "${name}" == "gui-smoke" && "${GUI_SERVER_READY}" -eq 0 ]]; then
    rows+=("<tr class=\"fail\"><td>gui-smoke</td><td class=\"status\">$(status_label FAIL)</td><td><pre>GUI server unavailable</pre></td></tr>")
    continue
  fi
  if [[ "${name}" == "gui-smoke" && "${GUI_URL}" == "https://localhost:8765" && "${STARTED_LOCAL_SERVER}" -eq 1 ]]; then
    cmd="node tools/playwright-smoke.mjs ${GUI_URL}"
  fi
  run_test "${name}" "${cmd}"
done

cat > "${REPORT_FILE}" <<HTML
<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Santron Test Report</title>
  <style>
    :root { color-scheme: light; --pass: #0f766e; --fail: #b91c1c; --bg: #f6f7f9; --panel: #ffffff; --text: #16202a; --muted: #5b6470; --line: #d8dde3; }
    body { margin: 0; font: 14px/1.5 system-ui, sans-serif; background: var(--bg); color: var(--text); }
    main { max-width: 1200px; margin: 0 auto; padding: 24px; }
    h1 { margin: 0 0 8px; font-size: 24px; }
    .meta { color: var(--muted); margin-bottom: 20px; }
    table { width: 100%; border-collapse: collapse; background: var(--panel); border: 1px solid var(--line); }
    th, td { vertical-align: top; border-bottom: 1px solid var(--line); padding: 12px; text-align: left; }
    th { background: #eef1f5; font-weight: 600; }
    tr.pass td.status { color: var(--pass); font-weight: 700; }
    tr.fail td.status { color: var(--fail); font-weight: 700; }
    pre { margin: 0; white-space: pre-wrap; word-break: break-word; font: 12px/1.45 ui-monospace, SFMono-Regular, Consolas, monospace; }
    .summary { display: inline-block; padding: 6px 10px; border-radius: 6px; background: #e8eef7; margin-bottom: 16px; }
    .pass .status { color: var(--pass); }
    .fail .status { color: var(--fail); }
  </style>
</head>
<body>
  <main>
    <h1>Santron Test Report</h1>
    <div class="meta">
      Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
    </div>
    <div class="summary">Overall: $( [ "${overall_pass}" -eq 1 ] && echo PASS || echo FAIL )</div>
    <table>
      <thead>
        <tr><th>Suite</th><th>Status</th><th>Output</th></tr>
      </thead>
      <tbody>
        ${rows[*]}
      </tbody>
    </table>
  </main>
</body>
</html>
HTML

printf 'Report written to %s\n' "${REPORT_FILE}"
printf 'Log written to %s\n' "${LOG_FILE}"

exit "${overall_pass}"
