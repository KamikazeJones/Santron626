# Lokales CA-Zertifikat installieren

Santron626 nutzt fuer den lokalen HTTPS-Server ein eigenes CA-Zertifikat. Dieses Zertifikat muss in dem Zertifikatsspeicher installiert werden, den der jeweilige Client benutzt.

Wichtig: Installiert wird das CA-Zertifikat:

```text
certs/santron626-dev-ca.crt
```

Nicht installieren als vertrauenswuerdige CA:

```text
certs/santron626-local.crt
```

`santron626-local.crt` ist nur das Server-Zertifikat fuer `https://192.168.2.122:8765`.

## Zertifikate im Projekt

```text
certs/santron626-dev-ca.crt     Lokale CA, diese Datei importieren
certs/santron626-dev-ca.key     Privater CA-Schluessel, nicht verteilen
certs/santron626-local.crt      Server-Zertifikat
certs/santron626-local.key      Privater Server-Schluessel
```

Das Server-Zertifikat ist gueltig fuer:

```text
DNS:localhost
IP:127.0.0.1
IP:192.168.2.122
```

## WSL / OpenSSL

Dieser Schritt sorgt dafuer, dass Tools wie `openssl` und `curl` in WSL der lokalen CA vertrauen.

```bash
sudo cp certs/santron626-dev-ca.crt /usr/local/share/ca-certificates/santron626-dev-ca.crt
sudo update-ca-certificates
```

Pruefen:

```bash
openssl s_client \
  -connect 192.168.2.122:8765 \
  -servername 192.168.2.122 \
  -verify_return_error </dev/null
```

Erwartet:

```text
Verification: OK
Verify return code: 0 (ok)
```

## Playwright-Chromium / NSS

Playwrights Chromium nutzt nicht immer denselben Trust Store wie OpenSSL. Fuer Service Worker reicht es deshalb nicht unbedingt, dass `openssl` bereits `Verification: OK` meldet.

Zuerst `certutil` installieren:

```bash
sudo apt-get update
sudo apt-get install -y libnss3-tools
```

Dann die CA in den Benutzer-NSS-Store importieren:

```bash
mkdir -p "$HOME/.pki/nssdb"
certutil -N -d "sql:$HOME/.pki/nssdb" --empty-password
certutil -d "sql:$HOME/.pki/nssdb" -A \
  -t "C,," \
  -n "Santron626 Local Dev CA" \
  -i certs/santron626-dev-ca.crt
```

Pruefen:

```bash
certutil -d "sql:$HOME/.pki/nssdb" -L -n "Santron626 Local Dev CA"
```

In der Ausgabe sollten unter `Certificate Trust Flags` diese SSL-Flags stehen:

```text
Valid CA
Trusted CA
```

Danach den Service-Worker-Smoke ausfuehren:

```bash
npm run gui:smoke:sw -- https://192.168.2.122:8765
```

Erwartet ist eine aktive Service-Worker-Registration:

```text
Service workers: allow
Chromium ignore certificate errors: no
Service worker registrations: [{"scope":"https://192.168.2.122:8765/","active":true,...}]
```

## Windows

Fuer Browser oder Tools, die den Windows-Zertifikatsspeicher verwenden, `certs/santron626-dev-ca.crt` in den Speicher fuer vertrauenswuerdige Stammzertifizierungsstellen importieren.

Mit PowerShell als Administrator:

```powershell
Import-Certificate `
  -FilePath .\certs\santron626-dev-ca.crt `
  -CertStoreLocation Cert:\LocalMachine\Root
```

Ohne Administratorrechte kann der Import alternativ nur fuer den aktuellen Benutzer erfolgen:

```powershell
Import-Certificate `
  -FilePath .\certs\santron626-dev-ca.crt `
  -CertStoreLocation Cert:\CurrentUser\Root
```

## Android

Die Datei `certs/santron626-dev-ca.crt` auf das Android-Geraet kopieren und als CA-Zertifikat installieren.

Der genaue Menuepfad haengt vom Hersteller ab. Typische Pfade:

```text
Einstellungen -> Sicherheit -> Verschluesselung & Anmeldedaten -> Zertifikat installieren -> CA-Zertifikat
```

oder:

```text
Einstellungen -> Sicherheit und Datenschutz -> Weitere Sicherheitseinstellungen -> Zertifikat installieren
```

Danach Chrome neu starten und `https://192.168.2.122:8765` aufrufen.

## Fehlerbild

Wenn die Seite laedt, aber der Service Worker nicht registriert wird, sieht der Playwright-Smoke typischerweise so aus:

```text
An unknown error occurred when fetching the script.
Service worker registrations: []
```

Dann vertraut der Browser dem Zertifikat fuer normale Navigation eventuell noch nicht vollstaendig. Fuer Playwright-Chromium ist in diesem Fall meistens der NSS-Import aus dem Abschnitt `Playwright-Chromium / NSS` der fehlende Schritt.

Als lokaler Test-Fallback kann der SW-Smoke Zertifikatsfehler bewusst ignorieren:

```bash
npm run gui:smoke:sw -- https://192.168.2.122:8765 --ignore-certificate-errors
```

Das ist nuetzlich zur Diagnose, ersetzt aber nicht den CA-Import.
