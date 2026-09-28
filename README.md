# Santron626 Haltpunkt

Aktueller Stand:

- Die Sanyo-Extraktion läuft im Mathematics-Abschnitt.
- A-3 `Cosine rule and area of triangle` ist bereits umgesetzt.
- Die WSL-CA für `https://192.168.2.122:8765` ist jetzt vertrauenswürdig.
- `openssl s_client -connect 192.168.2.122:8765 -servername 192.168.2.122` meldet `Verification: OK`.
- Anleitung zur Installation der lokalen CA: `docs/local-ca-installation.md`.

Zusätzlicher neuer Arbeitsstand:

- Unter `java-ga/` gibt es jetzt einen ersten separaten Java-Prototyp für eine generische Bibliothek zur genetischen Suche.
- Der erste Beispiel-Fall ist das Schiebepuzzle:
  - generischer GA-Kern ohne Santron-Wissen
  - Puzzle-Domäne als getrennte Modellierung
  - CLI-Smoke über `./build.sh` und `./run.sh`
- Das CLI unterstützt derzeit:
  - `--size N` für ein zufällig aus der gelösten Lage erzeugtes, sicher lösbares `N x N`-Puzzle
  - `--seed N` für reproduzierbare Instanzen
  - `--board "..."` für explizite Startstellungen
  - `--generations N` und `--population N` für das Suchbudget
- Zusätzlich gibt es jetzt eine lokale HTTP-Laufzeit für adressierbare Suchläufe:
  - `./run.sh server`
  - `./run.sh start ...`
  - `./run.sh list`
  - `./run.sh status <runId>`
  - `./run.sh best <runId>`
  - `./run.sh pause <runId>`
  - `./run.sh resume <runId>`
  - `./run.sh stop <runId>`
- Die Puzzle-Bewertung enthält aktuell:
  - Manhattan-Distanz
  - Anzahl falsch platzierter Steine
  - Strafen für lange und unwirksame Zugfolgen
  - lokale Normalisierung von Zugfolgen (`UD`, `DU`, `LR`, `RL`)
- Der Java-Kern liefert inzwischen strukturierte Laufdaten:
  - Run-ID
  - Run-Status
  - Snapshots pro Generation
  - Score-Metriken
  - Diagnosehinweise
  - Populationshinweise wie Streuung, Unique-Ratio und Anzahl gelöster Kandidaten
- Der 4x4-Fall funktioniert als erster Smoke brauchbar.
- Der 5x5-Fall ist mit der aktuellen Repräsentation und Fitness noch deutlich zu schwach; der Solver ist dort noch kein überzeugender Problemlöser.

Nächster sinnvoller Schritt im Java-GA-Zweig:

1. Snapshot-Historie und Analyse-Endpunkte ergänzen, statt nur den letzten Snapshot anzubieten.
2. Daraus Hinweise ableiten, ob Fitness, Repräsentation oder Operatoren das Problem sind.
3. Erst danach weitere problemnahe Heuristiken oder automatische Empfehlungen ergänzen.

Letzter offener Schritt:

1. Den GUI-Smoke erneut gegen `https://192.168.2.122:8765` ausführen.
2. Prüfen, ob der Browser-Fehler wegen des Zertifikats jetzt weg ist.
3. Falls nötig, den Smoke ohne Service-Worker-Block testen.

CORDIC-Rechenmodell:

- `cordic.js` implementiert dezimale Näherungsverfahren mit den MCS7529-014-ROM-Konstanten.
- `cordic.html` direkt im Browser öffnen: Double-/BCD-Vergleich, Schrittauswahl, Animation, Vektor- und Konvergenzdiagramme.
- `cordic-bcd.js`: exakte Dezimalarithmetik mit begrenzten Mantissen und protokollierten Stellenverlusten; ein Präzisionsmodell.
- API, Verfahren und Grenzen: [docs/cordic.md](docs/cordic.md).
- Keine bitgenaue BCD-/Mikrocode-Emulation.
