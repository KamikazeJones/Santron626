# Santron-Search

## Ziel

Wir wollen ein allgemeines Werkzeug bauen, das kurze Santron-626-Programme fuer
klar beschriebene Teilaufgaben sucht.

Mastermind ist der erste konkrete Anwendungsfall, aber nicht das eigentliche
Ziel des Werkzeugs. Das Suchprogramm soll spaeter auch fuer andere kleine
Santron-Routinen nutzbar sein, zum Beispiel Minimum, Ziffernextraktion,
Nulltests oder kurze Bewertungsfunktionen.

Das wichtigste Ziel ist derzeit:

- `mastermind-16.sce` bleibt die funktionierende Referenz und wird nicht fuer
  Experimente veraendert.
- `mastermind-search.sce` ist nur die aktuelle Mastermind-Such- und
  Experimentierkopie.
- Der C-Sucher soll nicht fuer jede neue Suche manuell umprogrammiert werden.
  Eingangszustand, erwarteter Ausgangszustand und Testfaelle sollen von aussen
  beschrieben werden.

## Ausgangslage

`mastermind-16.sce` enthaelt die aktuell funktionierende Version des
Mastermind-Programms. Der geheime Code liegt codiert in `M8`, der Benutzer gibt
die Guess-Ziffern nacheinander ein, und nach der vierten Ziffer stehen die
Bewertungen in den Speichern:

- `M1`: schwarze Treffer, also richtige Ziffer an richtiger Position
- `M9`: weisse Treffer, also richtige Ziffer an falscher Position

Diese Datei ist stabil und soll nicht als Spielwiese fuer die Suche verwendet
werden.

`mastermind-search.sce` ist dagegen bewusst eine Suchkopie fuer diesen einen
Anwendungsfall. Sie darf geaendert werden und erzeugt `mastermind-search.lst`,
damit die stabile Datei `mastermind-16.lst` nicht versehentlich ueberschrieben
wird.

## Problem mit dem aktuellen Sucher

Der aktuelle Sucher `tools/stochastic-mastermind-step.c` enthaelt die konkrete
Mastermind-Aufgabe noch direkt im C-Code:

- feste Trainingsfaelle
- feste Anfangszustaende fuer `M0` bis `M9`
- feste Nachbedingungen
- feste Annahmen zum Programmende
- feste Sequenztests ueber vier Guess-Ziffern

Das war fuer die ersten Experimente ausreichend, ist aber kein gutes Werkzeug.
Fuer jede neue Suche muesste man sonst wieder den C-Code aendern.

Der naechste Sucher sollte deshalb allgemeiner heissen, zum Beispiel:

```text
tools/santron-search.c
```

## Gewuenschtes Werkzeugmodell

Die Suche soll in drei Teile getrennt werden.

### 1. Suchmotor

Der Suchmotor bleibt C-Code. Er kennt:

- die Santron-626-VM
- die erlaubten Tastenfolgen
- Mutationen
- Bewertungskosten
- Zufalls-Seed und Iterationszahl

Der Suchmotor soll aber moeglichst wenig ueber die konkrete Aufgabe wissen.
Mastermind-Regeln gehoeren nicht fest in den Suchmotor.

### 2. Target-Datei

Eine Target-Datei beschreibt, was ein gesuchter Programmblock leisten soll.

Sie enthaelt:

- Vorbedingungen
- Eingaben
- erwartete Nachbedingungen
- Testfaelle
- optional Sequenztests
- geschuetzte Speicherzellen, die unveraendert bleiben muessen
- erlaubte oder verbotene Tasten
- maximales Programmlaengen-Budget

Arbeitstitel:

```text
santron-search.target
```

Fuer den aktuellen Mastermind-Fall kann die konkrete Datei trotzdem
`mastermind-step.target` oder `mastermind-search.target` heissen. Wichtig ist,
dass das Format allgemein genug bleibt.

### 3. Kandidaten-Datei

Eine Kandidaten-Datei enthaelt einen konkreten Startpunkt oder eine gefundene
Routine. Sie ist fuer Menschen lesbar und kann mit `santron-cli.js` in ein
Listing uebersetzt werden.

Fuer Mastermind ist das aktuell:

```text
mastermind-search.sce
```

## Beispiel: Target fuer den aktuellen Bewertungsblock

Die aktuelle Teilaufgabe ist ein Schritt der Guess-Bewertung. Der Benutzer hat
gerade eine Guess-Ziffer eingegeben. Diese Ziffer steht im Display. Der
Programmblock soll daraus die Speicher fuer die Bewertung aktualisieren.

Vorbedingungen:

```text
M0 = 1
M1 = bisherige schwarze Treffer
M2 = aktuelle Position, 1 bis 4
M8 = codierter CODE
M9 = bisheriger Weiss-/Hit-Vorrat
X  = eingegebene Guess-Ziffer
```

Nachbedingungen:

```text
M1 = M1 + 1, falls die eingegebene Ziffer an Position M2 im CODE steht
M9 = M9 - 1, falls die eingegebene Ziffer nicht im CODE vorkommt
M9 = M9 - 1, falls die eingegebene Ziffer an Position M2 im CODE steht
M2 = M2 + 1
M8 bleibt unveraendert
Programm haelt mit R/S
```

Dabei gilt:

- `M1` zaehlt schwarze Treffer.
- `M9` startet mit `4`.
- Nach vier Ziffern enthaelt `M9` die Anzahl der weissen Treffer.

## Sequenztests

Ein einzelner Schritt kann zufaellig korrekt aussehen, obwohl das Programm nach
mehreren Guess-Ziffern nicht mehr funktioniert. Deshalb braucht die Suche
zusaetzlich Sequenztests.

Beispiele:

```text
CODE 6413, Guess 1234 -> schwarz 0, weiss 3
CODE 6413, Guess 6413 -> schwarz 4, weiss 0
CODE 1264, Guess 3514 -> schwarz 1, weiss 1
CODE 1264, Guess 1264 -> schwarz 4, weiss 0
```

Diese Tests pruefen nicht nur das Endergebnis, sondern auch, ob der Block nach
jeder eingegebenen Ziffer wieder in einem brauchbaren Zustand haelt.

## Vorschlag fuer eine einfache Target-Syntax

Die erste Version muss nicht allgemein perfekt sein. Sie soll nur genuegend
ausdruecken koennen, um konkrete Suchen nicht mehr in C fest zu verdrahten.

Moegliche Form:

```text
name mastermind-step
max-cells 72
append RCL 2 = R/S
seed STO 4 1 0 STO 6 ...

case
init M0=1 M1=0 M2=1 M8=1024030 M9=4
keys 1 R/S
expect M1=0 M2=2 M8=1024030 M9=4 pause=1 pending=none loop=continue

case
init M0=1 M1=0 M2=3 M8=1024030 M9=3
keys 3 R/S
expect M1=0 M2=4 M8=1024030 M9=3 pause=1 pending=none loop=continue
```

`keys` beschreibt absichtlich keine abstrakte Eingabe, sondern echte
Santron-Tasten. `1 R/S` bedeutet also: Taste `1` druecken, dann `R/S`
druecken. Dadurch muss der Sucher keine Sonderlogik fuer "Guess-Ziffern"
kennen, sondern fuehrt einfach Tastenfolgen auf der VM aus.

Sequenztests sind als naechster Ausbauschritt geplant. Sie sollen ebenfalls mit
echten Tastenfolgen arbeiten:

```text
sequence
init M0=1 M1=0 M2=1 M8=1024030 M9=4
keys 1 R/S 2 R/S 3 R/S 4 R/S
expect M1=0 M2=5 M8=1024030 M9=3 pause=1

sequence
init M0=1 M1=0 M2=1 M8=1024030 M9=4
keys 6 R/S 4 R/S 1 R/S 3 R/S
expect M1=4 M2=5 M8=1024030 M9=0 pause=1
```

Die Syntax darf spaeter noch geaendert werden. Wichtig ist zuerst die Trennung
zwischen Suchmotor und Suchziel.

## Umsetzungsstand

Stand: erster lauffaehiger Prototyp.

Neu angelegt:

```text
tools/santron-search.c
mastermind-search.target
extract-search.target
```

Der Prototyp kann:

- eine Target-Datei mit `name`, `max-cells`, `append`, `seed` und mehreren
  `case`-Bloecken lesen
- mit `prefix` feste Programmschritte vor dem Suchkandidaten ausfuehren, ohne
  sie bei den Kandidatenkosten mitzuzuzaehlen
- pro `case` einen Anfangszustand mit `init` setzen
- nicht in `init` genannte Speicher und X/Y automatisch mit mehreren
  unterschiedlichen Werten testen
- echte Santron-Tasten aus `keys` ausfuehren, zum Beispiel `1 R/S`
- erwartete Speicher- und VM-Zustaende mit `expect` pruefen
- einen Startkandidaten aus `seed` bewerten
- mit `--candidate "..."` eine Tastenfolge pruefen, ohne die Target-Datei zu
  aendern
- stochastisch mutieren und nach kuerzeren bzw. guenstigeren Kandidaten suchen
- mit `--restarts n` mehrere Suchlaeufe mit fortlaufenden Seeds in einem
  Programmlauf ausfuehren und den besten Kandidaten ueber alle Restarts hinweg
  behalten
- mit `--random-start` jeden Restart mit einem komplett neu gewuerfelten
  Kandidaten beginnen, statt immer vom Seed auszugehen
- ohne `--seed` automatisch die Unixzeit in Millisekunden als Startseed
  verwenden
- mit `--promote-mastermind` kuerzere gueltige Extract-Kandidaten direkt in
  ein intern gebautes Mastermind-Programm einsetzen und mit dem vollstaendigen
  C-Test validieren
- mit `--promotion-log datei` akzeptierte Promotions sofort dauerhaft
  protokollieren
- mit `--verbose` fehlerhafte Faelle diagnostizieren

Der erste Check gegen `mastermind-search.target` besteht:

```text
/tmp/santron-search --target mastermind-search.target --iterations 0 --verbose
cells=55 ops=38 exact=36/36 error=0 penalty=0 cost=55
```

Ein kurzer Suchlauf funktioniert ebenfalls:

```text
/tmp/santron-search --target mastermind-search.target --iterations 1000 --seed 23
```

`--seed 23` ist nur noetig, wenn ein Lauf reproduzierbar sein soll. Ohne
`--seed` verwendet das Programm automatisch die Unixzeit in Millisekunden:

```text
/tmp/santron-search --target mastermind-search.target --iterations 1000
```

Die Ausgabe zeigt dann zum Beispiel:

```text
seed=1782211290505 seed-source=unix-ms
```

Explizit geht das auch mit:

```text
/tmp/santron-search --target mastermind-search.target --iterations 1000 --seed-ms
```

Mehrere Restarts laufen direkt im Programm:

```text
/tmp/santron-search --target extract-search.target --iterations 100 --restarts 3 --seed 23
```

Das fuehrt drei Suchlaeufe mit den Seeds `23`, `24` und `25` aus. Am Ende gibt
das Programm ein gemeinsames `overall best` aus.

Kreativere Suche mit zufaelligen Startprogrammen:

```text
/tmp/santron-search --target score-block.target --random-start --random-min-cells 5 --random-max-cells 30 --iterations 100000 --restarts 1000
```

Damit startet jeder Restart mit einem neu gewuerfelten Kandidaten innerhalb des
angegebenen Zellbereichs. Der Sucher gibt trotzdem den normalen Seed als
`initial candidate` aus, verwendet ihn aber in den Restarts nicht als
Startpunkt. Dieser Modus ist deutlich ineffizienter, kann aber weiter vom
bisherigen Ansatz weg springen.

Fuer einen langen Lauf bietet sich zum Beispiel an:

```text
/tmp/santron-search --target extract-search.target --iterations 1000000 --restarts 1000 --promote-mastermind --promotion-log santron-promotions.log
```

Mit `--promote-mastermind` passiert zusaetzlich:

1. Ein kuerzerer Kandidat muss zuerst `extract-search.target` voll bestehen.
2. Dann baut der Sucher intern das komplette Mastermind-Programm aus Prefix,
   Extract-Kandidat und Bewertungsblock.
3. Danach laeuft der Full-Test ueber alle Kombinationen ohne doppelte Ziffern.
4. Nur bei Erfolg wird der Kandidat als neuer Seed fuer die weitere Suche
   uebernommen.

Mit `--promotion-log` wird jede akzeptierte Promotion sofort in eine Datei
geschrieben. Die Datei wird im Append-Modus geoeffnet, also nicht bei jedem
Start geloescht. Ein Eintrag enthaelt den Fundort im Suchlauf, die Laenge der
Extract-Routine, die Laenge des daraus gebauten Mastermind-Programms, die
Anzahl der Full-Test-Faelle, die Tastenfolge und ein nummeriertes Listing. Der
Sucher fuehrt nach jedem geschriebenen Eintrag ein `fflush()` aus und beim
Schliessen ebenfalls nochmal vor `fclose()`.

Beispielausgabe:

```text
promotion check at initial candidate: extract cells=22 mastermind cells=67 full-tested=129600 result=PASS
promotion accepted: new seed has 22 extract cells
```

Zweiter Target-Test: `extract-search.target` beschreibt eine allgemeine
Ziffernextraktion:

```text
M8 = Zahl
Display = zu extrahierende Stelle n
M7 = Ziffer an Stelle 10^n
```

Der Seed ist inzwischen eine 22-Zellen-Extraktionsroutine. Sie besteht die
strenge Schnittstelle, bei der nicht genannte Speicher beliebige Startwerte
haben duerfen:

```text
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7
```

Check:

```text
/tmp/santron-search --target extract-search.target --iterations 0 --verbose
cells=22 ops=16 exact=42/42 error=0 penalty=0 cost=22
```

Promotion-/Full-Test-Check:

```text
/tmp/santron-search --target extract-search.target --iterations 0 --promote-mastermind --seed 23
promotion check at initial candidate: extract cells=22 mastermind cells=67 full-tested=129600 result=PASS
```

Diese Routine ueberschreibt `M7` zunaechst mit `STO 7` und ist deshalb
unabhaengig vom alten Wert in `M7`. `M4` wird als Scratch verwendet und darf
zerstoert werden. `M0` bleibt fuer das Mastermind-Programm frei und kann weiter
als Konstante `1` dienen.

Eine fruehere 23-Zellen-Variante

```text
10^X / RCL 8 X<>Y + 9 M+ 6 10^X M* 6 + M+ 0 RCL 6 - RCL 6 = M- 0
```

besteht die Extraktion, wenn `M6` vor dem Aufruf bereits 0 ist und `M0`
ebenfalls 0 ist. Das aktuelle Target testet jedoch bewusst verschiedene
Startwerte fuer alle nicht genannten Speicher, weil sie nicht zur beschriebenen
Eingabeschnittstelle gehoeren.

Die 27-Zellen-Variante

```text
10^X STO 6 M- 6 / RCL 8 X<>Y + 9 M+ 6 10^X M* 6 + M+ 0 RCL 6 - RCL 6 = M- 0
```

setzt `M6` selbst zurueck, ist aber weiterhin vom alten Wert in `M0`
abhaengig. Sie ist also nur gueltig, wenn `M0=0` als Vorbedingung erlaubt wird.

Direktes Testen einer Kandidatenfolge:

```text
/tmp/santron-search --target extract-search.target --candidate "/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7" --iterations 0 --verbose
```

Dritter Target-Test: `score-block.target` beschreibt nur den
Mastermind-Bewertungsblock. Die feste Eingabe- und Extraktionsroutine steht in
`prefix`, der Loop- und Ergebnis-Halt in `append`. Mutiert und bewertet wird
dadurch nur der Score-Block:

```text
RCL 7 - +/- SKP RCL 0 M- 9 RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1 1 M+ 2
```

Das Target verwendet echte Vier-Ziffern-Sequenzen, damit Kandidaten auch den
Zustand zwischen den Guess-Ziffern korrekt hinterlassen muessen. Eine
scheinbar kuerzere 24-Zellen-Variante ohne `4 X<>Y` bestand einzelne
Step-Tests, scheiterte aber im kompletten Mastermind-Full-Test. Das
Sequenz-Target verwirft diese Variante.

Check:

```text
/tmp/santron-search --target score-block.target --iterations 0 --verbose
cells=26 ops=18 exact=18/18 error=0 penalty=0 cost=26
```

Kurzer Suchlauf:

```text
/tmp/santron-search --target score-block.target --iterations 1000 --seed 23
```

Offene Grenzen des Prototyps:

- Eine eigene `sequence`-Direktive ist noch nicht implementiert; Sequenzen
  koennen aber bereits als mehrere echte Tastenfolgen in `keys` modelliert
  werden, wenn `prefix` und `append` den Programmfluss passend einrahmen.
- Die Target-Syntax ist bewusst einfach und noch nicht stabil.
- Es gibt noch keine Ausgabe direkt als `.sce`.
- Der Sucher wird bisher nach `/tmp/santron-search` kompiliert; es gibt noch
  kein Build-Script.
- `mastermind-search.target` bleibt ein kleiner Trainings-/Target-Test; der
  vollstaendige Integrationscheck fuer Extract-Kandidaten laeuft ueber
  `--promote-mastermind`.

Zusaetzlich existiert ein separater Full-Test fuer `mastermind-search.lst`:

```text
tools/test-mastermind-search.c
```

Er testet Code und Guess ohne doppelte Ziffern:

```text
/tmp/test-mastermind-search mastermind-search.lst
PASS 129600 code/guess combinations; input halt pc=6 result halt pc=64
```

## Geplante Schritte

1. Erledigt: Eine kleine Target-Datei `mastermind-search.target` fuer den
   aktuellen Bewertungsblock entwerfen.
2. Erledigt: Einen allgemeinen Sucher `tools/santron-search.c` vorbereiten.
3. Erledigt: Im C-Sucher einen einfachen Parser fuer diese Target-Datei
   einbauen.
4. Teilweise erledigt: Die bisher fest eingebauten Trainingsfaelle in die
   Target-Datei verschieben.
5. Offen: Die fest eingebauten Sequenztests ebenfalls in die Target-Datei
   verschieben.
6. Offen: Den Sucher als normales Tool bauen koennen:

```text
tools/santron-search --target mastermind-search.target --seed 23 --iterations 500000 --restarts 100
```

7. Erledigt fuer Extract-Promotion: Gefundene Kandidaten werden intern in ein
   Mastermind-Programm eingesetzt und per Full-Test geprueft.
8. Gefundene Kandidaten nicht direkt in `mastermind-16.sce` schreiben, sondern
   zuerst in `mastermind-search.sce` testen.
9. Erst wenn ein Kandidat vollstaendig gegen die Testmenge besteht, wird
   entschieden, ob er in eine neue Version wie `mastermind-17.sce` uebernommen
   wird.

## Schutzregel

`mastermind-16.sce` und `mastermind-16.lst` gelten als stabile Referenz.
Suchlaeufe, Kandidaten und Zwischenstaende duerfen diese Dateien nicht
ueberschreiben.

Neue Experimente laufen ueber:

```text
mastermind-search.sce
mastermind-search.lst
mastermind-search.target
tools/santron-search.c
```

`tools/stochastic-mastermind-step.c` bleibt vorerst als alter, spezialisierter
Sucher erhalten. Neue Arbeit soll in `tools/santron-search.c` weitergehen.
