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

## Target-Spezifikation

Die Target-Datei beschreibt Programmrahmen, Eingaben, beobachtbares Verhalten
und dessen Bewertung. Sie beschreibt keinen Loesungsweg.

Globale Direktiven:

```text
name frei lesbarer Name
max-cells 30
prefix feste Santron-Tasten vor dem Kandidaten
append feste Santron-Tasten nach dem Kandidaten
seed bekannte Startfolge
```

`seed` wird vom flachen Sucher verwendet. Der Baum-Sucher ignoriert die Zeile.
`keys`, `prefix` und `append` enthalten immer echte Santron-Tasten, keine
abstrakten Eingaben.

Bewertungsregeln:

```text
score numeric raw
score numeric normalized
score exact-weight 0
score invalid 100
score timeout 1000
```

`numeric normalized` verwendet fuer numerische Checks:

```text
abs(actual - expected) / (1 + abs(expected))
```

`raw` verwendet die absolute Abweichung. `exact-weight` wird bei einer
numerischen Abweichung zusaetzlich zur Distanz addiert. Mit `0` bleibt der
Gradient ohne Alles-oder-nichts-Sprung erhalten. `invalid` bewertet NaN und
unendliche Werte. `timeout` bewertet einen Programmlauf, der nach 1000
VM-Schritten nicht beendet ist.

Ein Testfall beginnt mit `case` und genau einem Anfangszustand:

```text
case
init M0=1 M1=0 M2=1 M8=1024030 M9=4
```

Nicht in `init` genannte Speicher sowie `X` und `Y` werden mit drei
unterschiedlichen Varianten belegt. Damit darf ein Kandidat nicht
versehentlich von unbeschriebenen Anfangswerten abhaengen.

Ein Fall enthaelt einen oder mehrere aufeinanderfolgende Schritte:

```text
step
keys R/S 1 R/S
check M1=0 weight=5
check M2=2 weight=2
check pause=1 mode=exact

step
keys 2 R/S
check M1=0 weight=5
check M2=3 weight=2
```

Der VM-Zustand bleibt zwischen den Schritten erhalten. Dadurch kann ein Target
den Zustand nach jeder Eingabe bewerten, statt nur das Endergebnis zu sehen.

Syntax einer einzelnen Pruefung:

```text
check FIELD=WERT [mode=distance|exact] [weight=N] [tolerance=N]
```

Numerische Felder sind `X`, `Y`, `PC` und `M0` bis `M9`. Zustandsfelder sind
`pause`, `running`, `paren` und `exponent`. Zusaetzlich existieren:

```text
check pending=none
check pending-memory=none
check loop=continue
check loop=done
```

`mode=distance` ist fuer numerische Felder Standard. `mode=exact` ist fuer
Zustandsfelder Standard. `weight` multipliziert den Verlust; `tolerance`
definiert den Bereich mit Verlust null.

Die alte Kurzform bleibt gueltig:

```text
keys 1 R/S
expect M1=0 M2=2 pause=1 pending=none
```

Ohne `score`, `step` oder `check` verwendet der Baum-Sucher die bisherige
Legacy-Bewertung. Sobald eine dieser neuen Direktiven vorkommt, wird jeder
einzelne Check jedes Schritts und jeder Initialzustandsvariante zu einem
eigenen Lexicase-Ziel.

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
- mit `--soft-score` Teilerfolge innerhalb eines Testfalls bewerten, damit
  zufaellige Programme einen nutzbaren Gradienten bekommen
- mit `--local-goto` `GOTO`-Ziele ausserhalb des aktuell gesuchten Kandidaten
  hart verwerfen
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
/tmp/santron-search --target score-block.target --random-start --random-min-cells 5 --random-max-cells 30 --soft-score --local-goto --iterations 100000 --restarts 1000
```

Damit startet jeder Restart mit einem neu gewuerfelten Kandidaten innerhalb des
angegebenen Zellbereichs. Der Sucher gibt trotzdem den normalen Seed als
`initial candidate` aus, verwendet ihn aber in den Restarts nicht als
Startpunkt. Dieser Modus ist deutlich ineffizienter, kann aber weiter vom
bisherigen Ansatz weg springen.

`--soft-score` aendert nicht die Definition von "korrekt". Ein Kandidat ist
weiterhin erst dann gueltig, wenn alle Faelle exakt bestehen. Fuer die Suche
werden aber einzelne Erwartungen wie `M1`, `M2`, `M9`, `pause`, `pending` und
`loop` getrennt gezaehlt. Dadurch sieht der Sucher auch Fortschritt, wenn ein
zufaelliges Programm nur einzelne Speicher richtig setzt.

Die Programmlaenge zaehlt erst, wenn ein Kandidat korrekt ist. Fehlerhafte
Programme bekommen also keinen Bonus nur dadurch, dass sie kuerzer sind. Solange
noch Erwartungen verletzt werden, bestimmen Teilerfolge, Fehler, Penalties und
GOTO-Verstoesse die Kosten.

`--local-goto` ist fuer Routinen sinnvoll, die als Kandidat zwischen einem
festen `prefix` und einem festen `append` gesucht werden. Ein `GOTO` im
Kandidaten darf dann nur innerhalb des Kandidatenblocks landen:

```text
prefix_len <= ziel < prefix_len + kandidat_laenge
```

Spruenge in den Prefix, in den Append oder ganz aus dem Programm heraus werden
als `goto`-Verstoss gezaehlt. Der Kandidat bekommt dann Kosten `inf` und kann
nicht als beste Loesung akzeptiert werden. Das gilt auch im Beam-Modus.

`R/S` wird nicht zufaellig in Kandidaten eingefuegt. Haltepunkte gehoeren in
`prefix` oder `append`, nicht in den mutierten Routineblock. Manuell angegebene
Kandidaten duerfen `R/S` weiterhin enthalten, falls das fuer ein anderes Target
bewusst gewuenscht ist.

Experimenteller Beam-Rollout-Modus:

```text
/tmp/santron-search --target score-block.target --beam-rollout --beam-width 64 --beam-branch 16 --beam-rollouts 16 --beam-rounds 30 --random-min-cells 5 --random-max-cells 30 --soft-score --local-goto --seed-ms
```

Dieser Modus baut Programme von links nach rechts auf. Fuer jedes Praefix
werden mehrere zufaellige Fortsetzungen bis zur erlaubten Laenge ausprobiert.
Die besten Praefixe werden in den naechsten Runden weiterverfolgt.

Parameter:

```text
--beam-width n      Anzahl der Praefixe, die pro Runde behalten werden
--beam-branch n     zufaellige Erweiterungen pro behaltenem Praefix
--beam-rollouts n   zufaellige Vervollstaendigungen pro neuem Praefix
--beam-rounds n     maximale Anzahl Aufbau-Runden
--beam-anneal n     nach Beam-Ende die besten n Kandidaten kurz annealen
--beam-anneal-iterations n
                    Annealing-Schritte pro Beam-Kandidat, Standard 2000
```

Der bekannte Seed wird weiter als Referenz ausgegeben, aber der Beam-Modus
meldet am Ende einen eigenen `beam best`. Wenn dieser noch nicht korrekt ist,
endet das Programm mit Exit-Code 1.

Ohne Monte-Carlo-Fortsetzungen:

```text
/tmp/santron-search --target score-block.target --beam-rollout --beam-width 128 --beam-branch 32 --beam-rollouts 0 --beam-rounds 30 --random-max-cells 30 --soft-score --local-goto --seed-ms
```

Mit `--beam-rollouts 0` wird jedes Praefix direkt bewertet. Dann misst Runde 1
wirklich Programme mit einer Operation, Runde 2 Programme mit zwei Operationen
usw. `--random-max-cells` bleibt als Laengenlimit fuer Praefixe relevant;
`--random-min-cells` spielt in diesem direkten Modus keine Rolle.

Optional kann nach der Beam-Suche ein kurzes Annealing auf den besten
Beam-Kandidaten laufen:

```text
/tmp/santron-search --target score-block.target --beam-rollout --beam-width 1000 --beam-branch 32 --beam-rollouts 0 --beam-rounds 30 --beam-anneal 1000 --beam-anneal-iterations 2000 --random-max-cells 30 --soft-score --local-goto --seed-ms
```

Dabei wird jeder Kandidat mit seiner aktuellen Zelllaenge als Obergrenze
annealed. Der Kandidat darf also verbessert oder gekuerzt werden, aber nicht
laenger werden. `--beam-anneal 1000` kann nur so viele Kandidaten nachbearbeiten,
wie der letzte Beam tatsaechlich enthaelt; praktisch sollte `--beam-width` also
mindestens so gross sein.

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

## Syntaxbaum-Suche

Der experimentelle Sucher `tools/santron-tree-search.c` arbeitet nicht direkt
auf flachen Tastensequenzen, sondern erzeugt Programme als kleine
Syntaxbaeume. Diese Baeume werden anschliessend in Santron-Tasten kompiliert
und mit derselben VM-Bewertung gegen eine Target-Datei getestet.

Ein Smoke-Test:

```text
cc -std=c11 -O2 -Wall -Wextra tools/santron-tree-search.c -lm -o /tmp/santron-tree-search
/tmp/santron-tree-search --target score-block-tree.target --population 40 --generations 5 --depth 3 --elite 4 --max-cells 30 --seed 23
```

`score-block-tree.target` ist die Trace-Variante mit `step` und `check`.
`score-block.target` bleibt unveraendert, weil der flache
`tools/santron-search.c` die erweiterte Syntax noch nicht liest.

Die bekannte korrekte Routine kann ohne AST direkt gegen das Trace-Target
geprueft werden:

```text
/tmp/santron-tree-search --target score-block-tree.target --max-cells 30 --candidate "RCL 7 - +/- SKP RCL 0 M- 9 RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1 1 M+ 2"
candidate cells=26 exact=72/72 checks=468/468 error=0 penalty=0 cost=26 objectives=468
```

`max-cells` in der Target-Datei ist der Standardwert. Eine explizite Angabe
von `--max-cells` ueberschreibt ihn auch nach oben. Das ist insbesondere fuer
fehlerhafte Zwischenloesungen sinnvoll: Ihre Laenge beeinflusst die Bewertung
nicht, sie duerfen aber mehr Raum verwenden, um sich einer korrekten Funktion
anzunaehern.

Die harte Obergrenze ergibt sich aus dem 100-Zellen-Programmspeicher:

```text
100 - Prefix-Laenge - Append-Laenge
```

Hat das Target kein Append, reserviert der Sucher stattdessen eine Zelle fuer
das automatisch angehaengte `R/S`. Prefix, Kandidat und Append werden dadurch
niemals still abgeschnitten.

Die Ausgabe zeigt jeweils beides: zuerst die kompilierte Santron-Tastenfolge,
danach den Lisp-artigen Baum, aus dem sie entstanden ist. Der erste Prototyp
kennt Konstanten, `M0` bis `M9`, unaere Funktionen, binaere Ausdruecke,
Speicheroperationen, `if` und `seq`.

Das Blatt `X` bezeichnet den Displaywert, der beim Eintritt in den gesuchten
Kandidaten bereits vorhanden ist. Es kompiliert zu keiner Taste. Da dieser
Wert ohne zusaetzliches `STO` nicht erneut hergestellt werden kann, darf `X`
hoechstens einmal vorkommen und muss das zuerst ausgewertete Blatt sein:

```text
(- X M7)       -> - RCL 7 =
(SIN X)        -> SIN
(+ M7 X)       -> ungueltig
(+ X X)        -> ungueltig
```

Das erste Listing nutzt die Santron-Eingabereihenfolge: Der linke Operand `X`
steht schon im Display, danach folgen Operator und rechter Operand.

Der eingebaute Regressionstest prueft diese vier `X`-Faelle sowie beide
Laufzeitpfade eines kompilierten `if`:

```text
/tmp/santron-tree-search --self-test
tree self-test: PASS
```

Die erste eingebaute Bedingung ist der einstellige Operator `!=0`:

```text
(if (!=0 z) then)
(if (!=0 z) then else)
```

`!=0` ist bewusst kein allgemeiner zweistelliger Vergleich: Die Null ist fest
in der Bedeutung des Operators enthalten. `(!=0 z)` wird ohne Sprung zu
`z X^2 +/-` kompiliert. Fuer `z != 0` ist `z^2` positiv und wird durch `+/-`
negativ. Fuer `z = 0` bleibt das Ergebnis null. Damit kann `SKP` die Bedingung
direkt auswerten.

Der Ausdruck `(min x y)` wird mit der bekannten kompakten Routine uebersetzt:

```text
x - y = SKP C/CE + y =
```

Bei `x < y` ueberspringt `SKP` das Loeschen und `(x-y)+y` ergibt `x`.
Andernfalls wird das Zwischenergebnis geloescht und `0+y` ergibt `y`.
Da der zweite Operand zweimal ausgewertet wird, muss er nebenwirkungsfrei sein
und darf insbesondere nicht den einmaligen Eingangswert `X` enthalten.

Die Zweige sind derzeit Speicheranweisungen oder weitere `if`-Knoten. Nach
der Bedingung erzeugt der Compiler lokale Spruenge:

```text
condition SKP GOTO else-or-end then [GOTO end else]
```

Die zweistelligen `GOTO`-Adressen werden aus der Laenge des Target-Prefix und
der Position im kompilierten Kandidaten berechnet. Dadurch kann derselbe AST
in Targets mit unterschiedlich langen Prefixen verwendet werden.

Die Programmlaenge beeinflusst die Auswahl nur bei vollstaendig korrekten
Kandidaten. Solange Tests oder Einzelpruefungen fehlschlagen, sind zwei
ansonsten gleich bewertete Programme unabhaengig von ihrer Laenge gleich gut.

Standardmaessig verwendet die Baum-Suche Lexicase-Selektion:

```text
--selection lexicase
```

Im Legacy-Modus erhaelt jeder Target-Fall und jede Variante einen eigenen
Verlustwert. Bei Targets mit `step`/`check` wird jede einzelne Pruefung jedes
Schritts und jeder Variante zu einem eigenen Verlustwert. Bei jeder Elternwahl
werden diese Ziele zufaellig sortiert und schrittweise die jeweils besten
Kandidaten behalten. Dadurch bleiben Spezialisten fuer einzelne Teilprobleme
erhalten, auch wenn ihre Gesamtkosten noch nicht gut sind.
Mit `--selection aggregate` kann die fruehere Auswahl aus den insgesamt
besten Kandidaten verwendet werden.

`--immigrants n` ersetzt pro Generation die letzten `n` Nachkommen durch neue
Zufallsbaeume. Standard sind fuenf Prozent der Population. Das verhindert,
dass nach einem fruehen Plateau nur noch eng verwandte Baeume gekreuzt werden.

Die Variation verwendet standardmaessig:

```text
--point-mutation 50
--subtree-mutation 30
```

Die restlichen 20 Prozent sind Crossovers. Eine Punktmutation aendert nur eine
Konstante, Speicherzelle, Funktion, Rechenoperation oder Speicheroperation
eines vorhandenen AST. Dadurch koennen bereits weitgehend korrekte Programme
lokal verbessert werden. Die Teilbaum-Mutation bleibt fuer groessere
Strukturaenderungen erhalten.

### Rotierende Top-Programme

Der Baum-Sucher kann die besten ASTs jeder Generation als lesbare
S-Ausdruecke speichern und beim naechsten Start wieder als Population laden:

```text
--top-prefix top-programs
--top-files 3
--top-count 100
```

Damit entstehen rotierend:

```text
top-programs-0.ast
top-programs-1.ast
top-programs-2.ast
```

Jede nicht auskommentierte Zeile enthaelt genau einen vollstaendigen AST:

```lisp
(seq (M- M9 1) (if (!=0 M7) (M+ M1 1)) (M+ M2 1))
```

Nach dem Sortieren einer Generation wird die Datei
`generation % top-files` neu geschrieben. Vor dem Schliessen wird
`fflush()` ausgefuehrt. Wird der Prozess waehrend des Schreibens beendet, kann
hoechstens diese eine Datei unvollstaendig sein; die anderen Dateien enthalten
weiterhin fruehere Generationen.

Beim Start liest der Sucher alle Dateien mit dem angegebenen Prefix:

- Kommentar- und Leerzeilen werden ignoriert.
- Jede AST-Zeile wird separat geparst und strukturell validiert.
- Abgeschnittene oder ungueltige Zeilen werden verworfen.
- Jeder geladene AST wird mit dem aktuellen Target neu kompiliert und bewertet.
- Doppelte kompilierte Tastenfolgen werden entfernt.
- Freie Populationsplaetze werden anschliessend zufaellig gefuellt.

Die Dateien speichern bewusst keine alten Scores. Aenderungen an Target,
Gewichten oder Bewertungsregeln werden deshalb beim Wiederanlauf korrekt
beruecksichtigt.

Beispiel:

```text
/tmp/santron-tree-search \
  --target score-block-tree.target \
  --population 2000 \
  --generations 5000 \
  --depth 7 \
  --elite 100 \
  --immigrants 100 \
  --max-cells 60 \
  --top-prefix top-programs \
  --top-files 3 \
  --top-count 100 \
  --seed-ms
```

Der AST ist typisiert: Ausdruecke liefern einen Displaywert, Statements
veraendern Zustand oder Kontrollfluss. Ein `if` darf daher nur in `seq` oder
in einem anderen `if`-Zweig stehen, niemals als Operand von `+`, `*`, `min`
oder einer Funktion. Mutation und Crossover erhalten diese Rollen.

Binaere Ausdruecke werden kontextabhaengig uebersetzt. Ein binaerer Ausdruck
auf der rechten Seite eines anderen Operators erhaelt Klammern. Dadurch bleibt
die Bedeutung des AST erhalten und es entstehen keine nutzlosen Folgen wie
`= =`.

`seq` ist eine Anweisungsfolge, keine Aneinanderreihung beliebiger Ausdruecke.
Alle Kinder bis auf das letzte muessen deshalb Statements wie `STO`, `M+`,
`M-` oder `if` sein. Nur das letzte Kind darf ein normaler Ausdruck sein.
Damit werden unter anderem Folgen ausgeschlossen, die getrennte Konstanten zu
einer neuen Zahl verschmelzen oder einen Wert unmittelbar durch `RCL n`
ueberschreiben. Diese Form wird beim Erzeugen sowie nach Mutation und
Crossover geprueft.
