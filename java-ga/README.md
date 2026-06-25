# Java GA playground

Minimaler erster Schnitt für eine generische Bibliothek zur genetischen Suche.

Enthalten:

- generischer GA-Kern ohne Domänenwissen
- 24-Puzzle als erstes Beispielproblem
- CLI-Smoke mit textueller Ausgabe pro Generation
- einfacher Self-Test ohne externe Bibliotheken
- lokaler HTTP-Server für adressierbare Suchläufe
- CLI-Kommandos als HTTP-Client für laufende Runs

Aktueller Stand:

- Der erste vertikale Schnitt funktioniert:
  - generischer GA-Kern
  - Puzzle-Domäne
  - CLI-Build und CLI-Smoke
- Das 4x4-Puzzle funktioniert als erste Demo bereits brauchbar.
- Das 5x5-/24-Puzzle ist mit der aktuellen Modellierung noch nicht stark genug:
  - die Suche findet nicht zuverlässig Lösungen
  - Fitness und Repräsentation sind noch zu grob
  - der Solver ist dort aktuell eher ein Experimentiergerüst als ein guter Problemlöser

Aktuelle Modellierungsentscheidungen:

- Ein Individuum ist eine Zugfolge aus `U`, `D`, `L`, `R`.
- Zufallspuzzles werden aus der gelösten Lage durch `2 * size * size` Züge erzeugt und
  sind dadurch sicher lösbar.
- Explizite Bretter über `--board` werden per Parität auf Lösbarkeit geprüft.
- Die Bewertung nutzt derzeit:
  - Manhattan-Distanz
  - Anzahl falsch platzierter Steine
  - Strafen für Länge
  - Strafen für wirkungslose Züge
- Direkte Gegenpaare wie `UD`, `DU`, `LR`, `RL` werden vor der Bewertung lokal entfernt.

Wichtige offene Punkte:

- Die Bibliothek sollte als Nächstes bessere Analyse-/Diagnosedaten liefern:
  - Welche Score-Komponenten treiben die Auswahl?
  - Wie viele Mutationen sind neutral oder destruktiv?
  - Wo entstehen Plateaus?
- Erst auf Basis dieser Diagnosen sollten weitere problemnahe Heuristiken ergänzt werden.
- Für schwierigere 5x5-Fälle reicht die aktuelle Fitness sehr wahrscheinlich nicht aus.

Lokale HTTP-Laufzeit:

- `./run.sh server` startet einen lokalen HTTP-Server auf `127.0.0.1:8787`
- `./run.sh start ...` startet darüber einen Run
- `./run.sh list` listet bekannte Runs
- `./run.sh status <runId>` liefert Status und letzten Snapshot
- `./run.sh best <runId>` liefert den aktuell besten Kandidaten mit Metriken
- `./run.sh pause <runId>` pausiert einen laufenden Run an der nächsten Generationsgrenze
- `./run.sh resume <runId>` setzt einen pausierten Run fort
- `./run.sh stop <runId>` beendet einen laufenden oder pausierten Run

Beispiel:

```bash
cd java-ga
./run.sh server
./run.sh start --size 4 --generations 20 --population 50
./run.sh list
./run.sh status <runId>
./run.sh best <runId>
./run.sh pause <runId>
./run.sh resume <runId>
./run.sh stop <runId>
```

Alternativ direkt per HTTP:

```bash
curl http://127.0.0.1:8787/health
curl -X POST "http://127.0.0.1:8787/runs?size=4&generations=20&population=50"
curl http://127.0.0.1:8787/runs
curl http://127.0.0.1:8787/runs/<runId>
curl http://127.0.0.1:8787/runs/<runId>/best
```

Build:

```bash
cd java-ga
./build.sh
```

JavaDoc:

```bash
cd java-ga
chmod +x javadoc.sh
./javadoc.sh
```

Danach liegt die HTML-Doku unter `java-ga/javadoc/`.

Self-Test:

```bash
cd java-ga
./run.sh --self-test
```

Smoke-Run:

```bash
cd java-ga
./run.sh
```

Andere Größe:

```bash
cd java-ga
./run.sh --size 4
```

Explizites Startbrett:

```bash
cd java-ga
./run.sh --board "1 2 3 4 5 6 7 8 9 10 11 12 19 13 15 16 17 18 14 20 21 22 0 23 24"
```

Größeres Suchbudget:

```bash
cd java-ga
./run.sh --size 5 --generations 1000 --population 600
```

Hinweis:

- Standard ist `--size 5`.
- Standard ist `--generations 300`.
- Standard ist `--population 250`.
- Das Startbrett wird aus der gelösten Lage erzeugt und dann mit `2 * size * size`
  Zufallszügen gemischt.
- Dadurch bleibt das Puzzle per Konstruktion lösbar.
- Optional kann mit `--seed N` ein anderer Zufalls-Seed gewählt werden.
- Alternativ kann mit `--board` ein explizites Brett vorgegeben werden.
- Bei `--board` wird die Größe automatisch aus der Anzahl der Felder erkannt.
- Zugfolgen werden vor der Bewertung lokal vereinfacht, z. B. `UD`, `DU`, `LR`, `RL`.
- Wirkunglose Züge gehen negativ in die Bewertung ein.
