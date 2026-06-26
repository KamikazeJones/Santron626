# Java GA playground

Minimaler erster Schnitt für eine generische Bibliothek zur genetischen Suche.

## Architektur-Prinzipien

Dieses Projekt ist **keine Puzzle-Anwendung** mit etwas GA darunter. Dieses Projekt ist
eine **generische GA-Infrastruktur**, in die konkrete Probleme nur als austauschbare
Adapter eingehängt werden.

Nicht verhandelbare Architekturregeln:

- Die GA-Suche kennt keine Problem-Domain.
- Die Laufzeit-Infrastruktur kennt keine Problem-Domain.
- HTTP, CLI, Monitoring, Registry, Worker und Persistenz gehören zur Infrastruktur,
  nicht zur Puzzle-Domäne.
- Konkrete Probleme wie das Schiebe-Puzzle dürfen nur über klar definierte Interfaces
  oder Adapter an die generische Infrastruktur angeschlossen werden.
- Ein Klassenname oder Paketname, der Infrastruktur mit einer konkreten Domäne mischt,
  ist ein Architekturfehler und kein akzeptabler Zwischenzustand.

Aktueller Architekturstatus:

- Die frühere Kopplung von Puzzle-Domäne und HTTP-Laufzeit wurde aufgebrochen.
- Die generische Laufzeit liegt jetzt unter `dev.quassel.ga.runtime`.
- Das Puzzle hängt nur noch über `PuzzleRuntimeAdapter` an der Infrastruktur.
- Die Einstiegsschicht ist jetzt eine generische CLI unter `dev.quassel.ga.app`.
- Konkrete Probleme werden über `--problem <id>` ausgewählt, nicht über eigene CLI-Klassen.

Enthalten:

- generischer GA-Kern ohne Domänenwissen
- generische Runtime-Schicht für Worker und Registry
- 24-Puzzle als erstes Beispielproblem
- Travelling Salesman als zweites Beispielproblem
- CLI-Smoke mit textueller Ausgabe pro Generation
- einfacher Self-Test ohne externe Bibliotheken
- generischer Worker-HTTP-Server für adressierbare Suchläufe
- generischer Registry-HTTP-Server für aggregierte Sicht auf mehrere Worker
- CLI-Kommandos als HTTP-Client für laufende Runs

Aktueller Stand:

- Der erste vertikale Schnitt funktioniert:
  - generischer GA-Kern
  - generische Runtime-Infrastruktur
  - Puzzle-Adapter
  - CLI-Build und CLI-Smoke
- Das 4x4-Puzzle funktioniert als erste Demo bereits brauchbar.
- Das 5x5-/24-Puzzle ist mit der aktuellen Modellierung noch nicht stark genug:
  - die Suche findet nicht zuverlässig Lösungen
  - Fitness und Repräsentation sind noch zu grob
  - der Solver ist dort aktuell eher ein Experimentiergerüst als ein guter Problemlöser
- Diversität wird derzeit als `uniqueCandidateRatio` pro Generation mitgeführt.
  Das ist der Anteil einzigartiger Kandidaten-Fingerprints in der Population.

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

- Höchste Priorität:
  - Registry- und Worker-HTTP noch stärker um generische Transportmodelle herum härten
  - weitere problemunabhängige Infrastruktur-Schnittstellen definieren
  - die Problem-Registry später dynamisch statt statisch zusammensetzen
- Die Bibliothek sollte als Nächstes bessere Analyse-/Diagnosedaten liefern:
  - Welche Score-Komponenten treiben die Auswahl?
  - Wie viele Mutationen sind neutral oder destruktiv?
  - Wo entstehen Plateaus?
- Erst auf Basis dieser Diagnosen sollten weitere problemnahe Heuristiken ergänzt werden.
- Für schwierigere 5x5-Fälle reicht die aktuelle Fitness sehr wahrscheinlich nicht aus.

Lokale HTTP-Laufzeit:

- `./start.sh --problem ...` ist der einfache Einstieg:
  - startet bei Bedarf automatisch eine Registry auf Port `8788`
  - startet bei Bedarf automatisch einen passenden Worker ab Port `8789`
  - startet danach den gewünschten Run
- `./watch.sh` zeigt nur aktive Runs der Default-Registry auf Port `8788`, inklusive Request-/GA-Metadaten
- `./watch-all.sh` zeigt alle bekannten Runs der Default-Registry auf Port `8788`
- `./list.sh` listet alle Runs der Default-Registry auf Port `8788`
- `./status.sh <runId>` zeigt den vollständigen Status eines Runs auf der Default-Registry, inklusive Request-/GA-Metadaten
- `./best.sh <runId>` zeigt den aktuell besten Kandidaten eines Runs auf der Default-Registry
- `./pause.sh <runId>` pausiert einen Run auf der Default-Registry
- `./resume.sh <runId>` setzt einen pausierten Run auf der Default-Registry fort
- `./stop.sh <runId>` beendet einen Run auf der Default-Registry
- `./shutdown.sh` beendet alle über die Default-Registry bekannten Worker und danach die Default-Registry selbst
- `./run.sh start --problem ...` macht dasselbe ohne separates Einstiegsskript
- `./run.sh registry` startet die Registry manuell im Hintergrund auf Default-Port `8788`
- `./run.sh registry --port N` überschreibt den Registry-Port
- `./run.sh server --problem puzzle --registry-url URL` startet einen Worker manuell im Hintergrund
- `./run.sh server --problem puzzle` wählt automatisch den ersten freien Worker-Port ab `8789`
- `./run.sh server --problem puzzle --port N` überschreibt den Worker-Port
- `./run.sh server-status --port N` zeigt Status, Rolle und Runs eines Registry- oder Worker-Prozesses
- `./run.sh server-stop --port N` beendet einen Registry- oder Worker-Prozess
- `./run.sh server-info --port N` zeigt Rolle, PID, Host und Startzeit des Prozesses
- `./run.sh start --problem puzzle ... --port N` nutzt den Registry-Port `N`, startet bei Bedarf Infrastruktur und legt dort den Run an
- `./run.sh list --port N` listet bekannte Runs auf einem Worker oder aggregiert über die Registry
- `./run.sh status <runId> --port N` liefert Status und letzten Snapshot
- `./run.sh best <runId> --port N` liefert den aktuell besten Kandidaten mit Metriken
- `./run.sh watch <runId> --port N` zeigt Status und Bestwert fortlaufend im Terminal
- `./run.sh watch all --port N` zeigt alle Runs des angesprochenen Endpunkts; auf der Registry also workerübergreifend
- `./run.sh pause <runId>` pausiert einen laufenden Run an der nächsten Generationsgrenze
- `./run.sh resume <runId>` setzt einen pausierten Run fort
- `./run.sh stop <runId>` beendet einen laufenden oder pausierten Run

Beispiel:

```bash
cd java-ga
./start.sh --problem puzzle --size 4 --generations 20 --population 50
./watch.sh
./watch-all.sh
./list.sh
./status.sh <runId>
./best.sh <runId>
./run.sh watch <runId> --port 8788
./pause.sh <runId>
./resume.sh <runId>
./stop.sh <runId>
./shutdown.sh
```

TSP lokal:

```bash
cd java-ga
./run.sh --problem tsp --cities 40 --width 1000 --height 1000 --generations 500 --population 300
```

TSP über die Registry:

```bash
cd java-ga
./start.sh --problem tsp --cities 40 --width 1000 --height 1000 --generations 500 --population 300
./watch.sh
```

Alternativ direkt per HTTP:

```bash
curl http://127.0.0.1:8788/health
curl -X POST "http://127.0.0.1:8788/runs?problem=puzzle&size=4&generations=20&population=50"
curl http://127.0.0.1:8788/runs
curl http://127.0.0.1:8788/runs/<runId>
curl http://127.0.0.1:8788/runs/<runId>/best
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
./run.sh --self-test --problem puzzle
./run.sh --self-test --problem tsp
```

Smoke-Run:

```bash
cd java-ga
./run.sh --problem puzzle
```

Andere Größe:

```bash
cd java-ga
./run.sh --problem puzzle --size 4
```

Explizites Startbrett:

```bash
cd java-ga
./run.sh --problem puzzle --board "1 2 3 4 5 6 7 8 9 10 11 12 19 13 15 16 17 18 14 20 21 22 0 23 24"
```

Größeres Suchbudget:

```bash
cd java-ga
./run.sh --problem puzzle --size 5 --generations 1000 --population 600
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
