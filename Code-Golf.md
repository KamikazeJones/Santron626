# Code-Golf fuer Santron 626

Notizen zu kurzen Tastenfolgen, die sich in den Mastermind-Versuchen als
nuetzlich erwiesen haben.

## Vorzeichen als Bedingung

`SKP` ueberspringt die naechste Programmanweisung, wenn `X < 0` ist.

```text
...        # X berechnen
SKP
GOTO &ziel
```

Wenn `X < 0`, wird der `GOTO` uebersprungen. Wenn `X >= 0`, wird gesprungen.
Bei `GOTO` ueberspringt `SKP` die komplette dreiteilige Anweisung.

## Gleichheitstest

Aus `x - y` wird ein SKP-taugliches Ergebnis:

```text
= X^2 +/-
```

Ergebnis:

```text
x == y  ->  0
x != y  ->  negativ
```

Danach kann `SKP` auf Ungleichheit reagieren.

Beispiel fuer bedingtes Addieren bei Gleichheit:

```text
RCL 7 - RCL 2 = X^2 +/- SKP RCL 0 M+ 1
```

Vorbedingung: `M0 = 1`.

Wirkung:

```text
M7 == M2  -> M1 = M1 + 1
M7 != M2  -> M1 bleibt unveraendert
```

Warum das funktioniert:

```text
gleich:      X = 0, SKP ueberspringt nicht, RCL 0 laedt 1
ungleich:    X < 0, SKP ueberspringt RCL 0, es wird 0 addiert
```

## Nulltest

Aus `x` wird:

```text
X^2 +/-
```

Ergebnis:

```text
x == 0  ->  0
x != 0  ->  negativ
```

Wenn bei Nicht-Null addiert werden soll, braucht man einen kleinen Sprung:

```text
RCL 7 X^2 +/-
SKP
GOTO &zero
RCL 0
M+ 9
%zero
```

Vorbedingung: `M0 = 1`.

Wenn man einen Zaehler mit dem Maximalwert starten kann, ist oft die umgekehrte
Richtung kuerzer. Beispiel aus `mastermind-11.sce`:

```text
M9 = 4        # hit-Vorrat fuer vier Guess-Ziffern
```

Dann wird bei einer fehlenden Ziffer heruntergezaehlt:

```text
RCL 7 X^2 +/- SKP RCL 0 M- 9
```

Wirkung:

```text
M7 == 0  -> M9 = M9 - 1
M7 != 0  -> M9 = M9 - 0
```

Das spart gegenueber einem positiven Nicht-Null-Zweig mit `GOTO` drei
Programmschritte. Am Ende ist `M9` trotzdem wieder `hit = black + white`.

## Trunkieren mit `EXP 9`

Die Folge

```text
+ EXP 9 - EXP 9
```

schneidet die Nachkommastellen ab, solange der Wertbereich passt.

Beispiel:

```text
RCL 8 / RCL 4 10^X + EXP 9 - EXP 9 =
```

berechnet:

```text
floor(M8 / 10^M4)
```

Wichtig: Bei zusammengesetzten Ausdruecken muss nach der Trunkierung oft ein
`=` gesetzt werden, damit der Zwischenwert wirklich feststeht.

## Dezimalziffer extrahieren

### Historie der Extraktionsroutinen

| Schritte | Ziel | Routine | Bemerkung |
| ---: | --- | --- | --- |
| ca. 31 | `M7` | `RCL 8 / RCL 4 10^X + EXP 9 - EXP 9 = STO 7 / 1 0 + EXP 9 - EXP 9 = * 1 0 = M- 7` | Fruehe Trunkiermethode mit zwei Quotienten. |
| 27 | `M7` | `1 0 STO 6 RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7 M* 6 RCL 6 - RCL 6 = M- 7` | Stochastisch gefundene robuste Variante; Ergebnis an Stelle `10^M4`. |
| 33 | `M0` | `STO 4 1 0 STO 6 RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7 M* 6 RCL 6 - RCL 6 = M- 7 RCL 7 STO 0` | 27er-Routine angepasst auf Stelle im Display und Ergebnis in `M0`. |
| 23 | `M0` | `10^X / RCL 8 X<>Y + 9 M+ 6 10^X M* 6 + M+ 0 RCL 6 - RCL 6 = M- 0` | Kuerzer, aber nur gueltig mit Vorbedingung `M0=0` und `M6=0`. |
| 27 | `M0` | `10^X STO 6 M- 6 / RCL 8 X<>Y + 9 M+ 6 10^X M* 6 + M+ 0 RCL 6 - RCL 6 = M- 0` | Setzt `M6` selbst zurueck, braucht aber weiterhin `M0=0`. |
| 24 | `M7` | `/ RCL 8 X<>Y 10^X + EXP 9 + STO 6 STO 7 LN M* 6 RCL 6 - RCL 6 = M- 7` | Robuste `LN`-Variante; inzwischen ueberholt. |
| 22 | `M7` | `/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7` | Aktueller Stand fuer `mastermind-search`; besteht `extract-search.target` mit `42/42`. |

Aktuelle Schnittstelle fuer `extract-search.target`:

```text
M8 = Zahl
Display = zu extrahierende Stelle n
M7 = Ziffer an Stelle 10^n
```

Optimierte Routine fuer die Ziffer an Stelle `10^M4` aus `M8`.

```text
RCL 8 / RCL 4 10^X + EXP 9 - EXP 9 = STO 7
/ 1 0 + EXP 9 - EXP 9 = * 1 0 = M- 7
```

Ergebnis:

```text
M7 = floor(M8 / 10^M4) - 10 * floor(floor(M8 / 10^M4) / 10)
```

Kurz:

```text
M7 = Ziffer an Stelle 10^M4
```

Beispiel:

```text
CODE = 1264
M8 = 3040210
M4 = 4
M7 = 4
```

Codierungsregel: Die Stelle `10^d` enthaelt die Position der Ziffer `d` im
CODE. Fuer `1264` gilt also: `10^1 -> 1`, `10^2 -> 2`, `10^4 -> 4`,
`10^6 -> 3`.

Diese Variante ist kuerzer als die direkte Formel mit zwei Quotienten:

```text
floor(M8 / 10^M4) - 10 * floor(M8 / 10^(M4 + 1))
```

### Stochastisch gefundene 27-Schritt-Extraktion

Der stochastische Superoptimizer in `tools/stochastic-extract.c` fand eine
kuerzere Variante fuer die gleiche Aufgabe:

```text
1 0 STO 6
RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7
M* 6 RCL 6 - RCL 6 = M- 7
```

Listing:

```text
00 1
01 0
02 STO
03 6
04 RCL
05 8
06 /
07 RCL
08 4
09 10^X
10 +
11 EXP
12 0
13 9
14 +
15 STO
16 7
17 M*
18 6
19 RCL
20 6
21 -
22 RCL
23 6
24 =
25 M-
26 7
```

Wirkung:

```text
M4 = n
M8 = codierter CODE
M7 = Ziffer an Stelle 10^n
```

Erhalten bleiben `M0`, `M1`, `M2`, `M4`, `M8`, `M9`. `M6` wird intern auf
`10` gesetzt und danach als Scratch-Speicher benutzt. Dadurch ist die Routine
wiederholt aufrufbar, solange `M6` im aufrufenden Programm frei ist.

Arbeitsweise:

```text
q = M8 / 10^M4
T = 1E9 + floor(q)
M7 = T
M6 = 10*T
U = round10(T + M6) - M6
M7 = T - U
```

Durch die 10-stellige interne Genauigkeit verliert `round10(T + M6)` die letzte
Ziffer von `T`. Deshalb ist `T - U` genau die letzte Ziffer von `floor(q)`,
also die gesuchte Dezimalziffer.

Die erste vom Optimizer gefundene Fassung enthielt `EXP GOTO 0 9`, `M+ 7`,
`+/-`, `SKP`, `RCL 7` und ein abschliessendes `=`. Beim manuellen Nachpruefen
zeigte sich: Diese Teile sind hier nicht noetig oder durch gleich lange,
sauberere Formen ersetzbar. Gegenueber der bisherigen 31-Schritt-Extraktion
spart die bereinigte Variante vier Programmschritte.

Die Routine wurde mit der C-VM auf mehreren codierten CODE-Werten validiert und
gegen den JS-Core stichprobenartig geprueft.

### Kuerzere Extraktion mit Stelle im Display

Fuer die Schnittstelle

```text
M8 = Zahl
X  = zu extrahierende Stelle n
M7 = Ziffer an Stelle 10^n
```

wurde eine 22-Zellen-Routine gefunden, die die strenge Schnittstelle besteht:

```text
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7
```

Sie ueberschreibt `M7` zunaechst mit `STO 7`, ist also unabhaengig vom alten
Inhalt von `M7`. `M4` wird als Scratch verwendet und darf zerstoert werden.
`M0` bleibt frei fuer die Konstante `1` im Mastermind-Bewertungsblock. Gegen
`extract-search.target` besteht sie mit `42/42` Faellen.

Eine fruehere 23-Zellen-Variante

```text
10^X / RCL 8 X<>Y + 9 M+ 6 10^X M* 6 + M+ 0 RCL 6 - RCL 6 = M- 0
```

setzt voraus, dass `M6` und `M0` vor dem Aufruf
`0` sind. Sie verwendet `M+ 0` und `M- 0`, schreibt das Ergebnis also relativ
zum alten Inhalt von `M0`.

Wenn nur `M6` nicht vorgegeben werden soll, kann `M6` am Anfang aus dem gerade
berechneten `10^X` heraus geloescht werden:

```text
10^X STO 6 M- 6 / RCL 8 X<>Y + 9 M+ 6 10^X M* 6 + M+ 0 RCL 6 - RCL 6 = M- 0
```

Diese Variante braucht 27 Zellen, setzt aber immer noch voraus, dass `M0` vor
dem Aufruf `0` ist.

Vor der 22-Zellen-Fassung war die robuste 33-Zellen-Fassung der beste Seed:

```text
STO 4 1 0 STO 6 RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7 M* 6 RCL 6 - RCL 6 = M- 7 RCL 7 STO 0
```

Sie ueberschreibt `M0` am Ende mit `STO 0`, ist aber inzwischen laenger als der
neue `EXP 9`/`STO 4`-Trick.

## Hit statt White zaehlen

Bei Mastermind ist es oft kuerzer, nicht `white` direkt zu zaehlen.

Stattdessen:

```text
M1 = black
M9 = hit = black + white
```

Dann gilt am Ende:

```text
white = M9 - M1
```

Vorteil: Fuer jede Guess-Ziffer muss nur getestet werden, ob die Ziffer
ueberhaupt im CODE vorkommt. Der aufwendigere Fall `nicht schwarz, aber
vorhanden` entfaellt.

## Externe Initialisierung

Wenn Programmschritte knapp sind, koennen Werte vor Programmstart von Hand in
Speicher gelegt werden.

Typische Kandidaten:

```text
M0 = 1
M1 = 0        # black
M2 = 1        # Position
M8 = CODE     # codiert
M9 = 0        # hit
```

Das spart Programmschritte, kostet aber Bedienkomfort.

## Destruktive Routinen vermeiden

Eine Routine, die den CODE-Speicher zerlegt, wirkt zuerst kurz, wird aber teuer,
wenn sie mehrfach pro Guess gebraucht wird.

Besser:

```text
M8 = CODE bleibt unveraendert
M7 = Ergebnis der Extraktion
```

Die Trunkiermethode hat hier einen Vorteil gegenueber der Subtraktionsschleife:
Sie laesst `M8` unveraendert.

## Schleifenstart geschickt platzieren

Manchmal kann ein erfolgreicher Zweig vor den Schleifenkopf gesetzt werden.
Dann faellt der Zweig ohne zusaetzlichen `GOTO` direkt in die Schleife hinein.

Das wurde in `mastermind-09.sce` getestet. Es spart einen Sprung, macht den
Ablauf aber schwerer lesbar.

## Minimum aus zwei Zahlen

Kompakte Routine, wenn der Ausdruck `x - y` bereits vorbereitet ist:

```text
STO 1 = SKP C/CE + RCL 1 =
```

Interpretation:

```text
x - y < 0  -> x ist kleiner
x - y >= 0 -> y ist kleiner oder gleich
```

Diese Routine war fuer die doppelte-Ziffern-Variante interessant, weil dort pro
Farbe `min(Code-Anzahl, Guess-Anzahl)` gebraucht wird.

### Stochastisch gefundene 13-Schritt-Variante

Der stochastische Superoptimizer in `tools/stochastic-min.c` fand ausgehend von
der schon gegolften Routine eine kuerzere Variante:

```text
RCL 0 - M+ 2 RCL 1 = SKP M- 2 RCL 2
```

Listing:

```text
00 RCL
01 0
02 -
03 M+
04 2
05 RCL
06 1
07 =
08 SKP
09 M-
10 2
11 RCL
12 2
```

Wirkung:

```text
M0 = x
M1 = y
M2 = 0  # Vorbedingung, Scratch-Speicher

X = min(x, y)
```

Warum die Variante kuerzer ist: Sie ersetzt das Speichern mit `STO 2` durch
`M+ 2`. Dadurch spart sie eine Zelle gegenueber der bisherigen 14-Zellen-
Routine. Der Preis ist die Vorbedingung `M2 = 0`.

Die Routine wurde mit der C-VM und gegen den JS-Core geprueft. Sie ist fuer
`M2 = 0` auf dem Gitter `-9..9` gueltig. Wenn `M2` nicht 0 ist, liefert sie
falsche Werte, weil `M+ 2` den alten Speicherinhalt einbezieht.

## Aus Hit wird White

In `mastermind-13.sce` startet `M9` wie vorher bei 4. Bei einer nicht im CODE
vorkommenden Ziffer wird 1 abgezogen. Damit waere `M9` zunaechst `black+white`.

Im Black-Zweig wird dieselbe 0/1-Maske weiterbenutzt:

```text
RCL 7 - RCL 2 = X^2 +/- SKP RCL 0 M+ 1 M- 9
```

Wenn `M7 == M2`, dann ist `X=1` nach `RCL 0`, also wird `M1` erhoeht und `M9`
um 1 reduziert. Wenn `M7 != M2`, ueberspringt `SKP` nur `RCL`; die folgende
Speicherziffer `0` setzt `X=0`, also bleiben `M1` und `M9` unveraendert.

Dadurch enthaelt `M9` am Ende direkt `white`, nicht mehr `black+white`.

Der finale Schleifentest wurde umgedreht:

```text
4 - RCL 2 = SKP GOTO 0 0
```

Fuer Position 1..3 ist das Ergebnis nicht negativ und `GOTO 00` holt die
naechste Ziffer. Nach Position 4 ist das Ergebnis negativ, `SKP` ueberspringt
den dreizelligen Sprung und das Programm faellt direkt in die Ausgabe:

```text
RCL 1 R/S RCL 9 R/S
```

Erster Halt zeigt `black`, der zweite Halt zeigt `white`.
