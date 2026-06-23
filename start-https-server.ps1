$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$serverScript = Join-Path $repoRoot "https-server.py"
$certFile = Join-Path $repoRoot "certs/santron626-local.crt"
$keyFile = Join-Path $repoRoot "certs/santron626-local.key"

if (-not (Test-Path $serverScript)) {
  throw "Server script not found: $serverScript"
}

if (-not (Test-Path $certFile)) {
  throw "Certificate not found: $certFile"
}

if (-not (Test-Path $keyFile)) {
  throw "Key not found: $keyFile"
}

$python = $null
foreach ($candidate in @("py", "python", "python3")) {
  $command = Get-Command $candidate -ErrorAction SilentlyContinue
  if ($command) {
    $python = $candidate
    break
  }
}

if (-not $python) {
  throw "No Python interpreter found. Tried: py, python, python3"
}

Set-Location $repoRoot
Write-Host "Starting HTTPS server from $repoRoot"
& $python $serverScript

