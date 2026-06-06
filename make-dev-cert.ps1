param(
  [string]$IpAddress = "192.168.2.122"
)

$ErrorActionPreference = "Stop"

$certDir = Join-Path $PSScriptRoot "certs"
New-Item -ItemType Directory -Force -Path $certDir | Out-Null

$openssl = (Get-Command openssl -ErrorAction Stop).Source
$caKey = Join-Path $certDir "santron626-dev-ca.key"
$caCert = Join-Path $certDir "santron626-dev-ca.crt"
$serverKey = Join-Path $certDir "santron626-local.key"
$serverCsr = Join-Path $certDir "santron626-local.csr"
$serverCert = Join-Path $certDir "santron626-local.crt"
$serverExt = Join-Path $certDir "santron626-local.ext"

@"
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage=digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=@alt_names

[alt_names]
DNS.1=localhost
IP.1=127.0.0.1
IP.2=$IpAddress
"@ | Set-Content -Path $serverExt -Encoding ascii

if (!(Test-Path $caKey) -or !(Test-Path $caCert)) {
  & $openssl genrsa -out $caKey 4096
  & $openssl req -x509 -new -nodes -key $caKey -sha256 -days 3650 -out $caCert -subj "/CN=Santron626 Local Dev CA"
}

& $openssl genrsa -out $serverKey 2048
& $openssl req -new -key $serverKey -out $serverCsr -subj "/CN=$IpAddress"
& $openssl x509 -req -in $serverCsr -CA $caCert -CAkey $caKey -CAcreateserial -out $serverCert -days 825 -sha256 -extfile $serverExt

Write-Host "Created local HTTPS certificate for $IpAddress"
Write-Host "Server cert: $serverCert"
Write-Host "Server key:  $serverKey"
Write-Host "Install this CA certificate on Android as a trusted CA:"
Write-Host "  $caCert"
