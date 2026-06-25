# Java GA playground

Minimaler erster Schnitt für eine generische Bibliothek zur genetischen Suche.

Enthalten:

- generischer GA-Kern ohne Domänenwissen
- 24-Puzzle als erstes Beispielproblem
- CLI-Smoke mit textueller Ausgabe pro Generation
- einfacher Self-Test ohne externe Bibliotheken

Build:

```bash
cd java-ga
./build.sh
```

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
