# Santron626 Haltpunkt

Aktueller Stand:

- Die Sanyo-Extraktion läuft im Mathematics-Abschnitt.
- A-3 `Cosine rule and area of triangle` ist bereits umgesetzt.
- Die WSL-CA für `https://192.168.2.122:8765` ist jetzt vertrauenswürdig.
- `openssl s_client -connect 192.168.2.122:8765 -servername 192.168.2.122` meldet `Verification: OK`.
- Anleitung zur Installation der lokalen CA: `docs/local-ca-installation.md`.

Letzter offener Schritt:

1. Den GUI-Smoke erneut gegen `https://192.168.2.122:8765` ausführen.
2. Prüfen, ob der Browser-Fehler wegen des Zertifikats jetzt weg ist.
3. Falls nötig, den Smoke ohne Service-Worker-Block testen.
