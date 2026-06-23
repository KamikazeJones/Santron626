# Mastermind auf dem Santron 626

## Zielbild

Das erste lauffaehige Ziel soll bewusst einfach bleiben. Der Rechner erzeugt den
Code nicht selbst, sondern der erste Spieler gibt den geheimen Code vor dem Spiel
ein. Danach startet das Programm, der zweite Spieler gibt einen Versuch ein, und
das Programm gibt die Bewertung aus.

Das passt zur erinnerten Spielweise:

1. Der Code wird vom ersten Spieler eingegeben.
2. Das Programm wird gestartet.
3. Der zweite Spieler gibt einen geratenen Code ein.
4. Das Programm antwortet mit der Bewertung.

## Vereinfachung fuer die erste Version

Eine zwischenzeitliche Idee war, Code und Guess als einzelne Ziffern in
separaten Speichern abzulegen, um die Zerlegung von `1234` in Einzelziffern zu
vermeiden.

Diese Idee wird vorerst verworfen. Der Grund ist nicht die Eingabe, sondern die
Bewertung: Ohne indirekte Adressierung lassen sich die vier Positionen dann nicht
mehr sauber in einer echten Schleife abarbeiten. Statt einer kompakten
Positionsschleife waeren vier fest verdrahtete Vergleiche noetig:

```text
RCL 0 gegen RCL 4
RCL 1 gegen RCL 5
RCL 2 gegen RCL 6
RCL 3 gegen RCL 7
```

Der Vorteil der einfacheren Eingabe wuerde dadurch weitgehend wieder verloren
gehen.

Aktuelle Arbeitshypothese:

- Code und Guess bleiben als gepackte vierstellige Zahlen gespeichert.
- Die Bewertung laeuft in zwei Phasen:
  `1.` schwarze Treffer positionsweise bestimmen,
  `2.` nicht-schwarze Reste in Vorkommens-Arrays sammeln und daraus die weissen
  Treffer berechnen.
- Die eigentliche Schleife soll also erhalten bleiben; optimiert werden muessen
  vor allem die Ziffernextraktion und die Restcodierung.

## Bewertung

Schwarze Treffer sind die einfachen Faelle: Code-Ziffer und Guess-Ziffer an
derselben Position werden verglichen. Bei Gleichheit wird der schwarze Zaehler
erhoeht.

Ein Python-Prototyp fuer diesen Bewertungs-Algorithmus existiert in
`mastermind-bewertung.py`. Er demonstriert insbesondere den Fall, dass doppelte
Ziffern im Code zulaessig sind und trotzdem nicht zu viele weisse Treffer
gezaehlt werden.

Fuer weisse Treffer brauchen wir pro Farbe die Anzahl der uebrig gebliebenen
Code-Ziffern und Guess-Ziffern. Pro Farbe wird dann das Minimum dieser beiden
Anzahlen addiert.

Der wichtige Trick ist, die nicht-schwarzen Ziffern nicht paarweise direkt zu
vergleichen, sondern als kleine Vorkommens-Arrays zu zaehlen. Das ist noetig,
wenn Ziffern mehrfach vorkommen duerfen.

Beispiel:

```text
Code:  1 1 2 3
Guess: 1 2 2 1
```

Zuerst werden schwarze Treffer entfernt. Position 1 ist schwarz, also bleibt:

```text
Code-Rest:  1 2 3
Guess-Rest: 2 2 1
```

Jetzt wird fuer jede Ziffer 1 bis 6 gezaehlt, wie oft sie im jeweiligen Rest
vorkommt:

```text
Ziffer:       1 2 3 4 5 6
Code-Rest:    1 1 1 0 0 0
Guess-Rest:   1 2 0 0 0 0
Minimum:      1 1 0 0 0 0
```

Die Summe der Minima ist die Anzahl weisser Treffer. Im Beispiel sind das
`1 + 1 = 2` weisse Treffer.

Ohne diese Vorkommens-Arrays wuerde man bei doppelten Ziffern leicht zu viele
weisse Treffer zaehlen. Eine Guess-Ziffer darf nur so oft zaehlen, wie sie im
Code-Rest noch vorhanden ist.

Auf dem Santron kann so ein Array platzsparend als Dezimalzahl codiert werden.
Die Stelle `10^n` zaehlt dann die Vorkommen der Ziffer `n`.

```text
1 kommt einmal vor:    +10
2 kommt zweimal vor:   +200
3 kommt einmal vor:    +1000
```

Fuer den Code-Rest `1 2 3` entsteht also `1110`, fuer den Guess-Rest `2 2 1`
entsteht `210`. Danach kann man pro Ziffer die jeweilige Dezimalstelle
extrahieren und mit der Minimum-Routine auswerten.

## Minimum-Routine

Es gibt eine sehr kompakte Routine, um aus zwei Zahlen das Minimum zu bestimmen.
Die Routine erwartet, dass der Ausdruck `x - y` bereits vorbereitet ist.

```text
STO 1 = SKP C/CE + RCL 1 =
```

Bedeutung:

- Vor der Routine steht ein Ausdruck der Form `x - y`.
- `STO 1` speichert den rechten Operanden `y`.
- `=` berechnet `x - y`.
- Ist das Ergebnis negativ, ueberspringt `SKP` die naechste Taste.
- Bei `x >= y` loescht `C/CE` das Ergebnis auf `0`.
- Danach wird `+ RCL 1 =` ausgefuehrt.

Ergebnis:

```text
min(x, y)
```

Fallunterscheidung:

```text
x < y:   x - y + y = x
x >= y:  0 + y     = y
```

Diese Routine ist fuer die weisse Mastermind-Bewertung wichtig, weil dort fuer
jede Farbe `min(Anzahl im Code, Anzahl im Guess)` gebraucht wird.

## Aktueller Stand

Die aktuell verwendete GUI-Version ist `mastermind-16.lst`. Sie ist als
Repository-Listing in der Santron626-GUI eingetragen.

Der Ablauf ist jetzt bewusst simpel:

1. Der erste Spieler versteckt den geheimen Code vor dem Start in `M8`.
2. Vor dem ersten Lauf setzt der Benutzer auch `M9` auf `4`.
3. Das Programm setzt `M0` und `M2` beim Start automatisch auf `1`.
4. Das Programm laeuft an.
5. Der zweite Spieler gibt die vier Guess-Ziffern nacheinander ein.
6. Nach der vierten Ziffer haelt das Programm an.
7. Die Bewertung hat dann zwei Teile:
   `schwarz` bedeutet: richtige Ziffer an richtiger Stelle.
   `weiß` bedeutet: richtige Ziffer, aber an falscher Stelle.
8. `RCL 1` zeigt die Anzahl der schwarzen Treffer.
9. `RCL 9` zeigt die Anzahl der weißen Treffer.
10. Fuer den naechsten Guess setzt der Benutzer die Speicher selbst zurueck:

```text
C/CE STO 1 4 STO 9
```

Damit sind die Bewertungswerte fuer den naechsten Durchlauf wieder sauber:
`M1` wird auf `0` gesetzt, `M9` wieder auf `4`.
Das Programm setzt `M0` und `M2` automatisch auf `1` zurueck und springt dann
zur naechsten Guess-Eingabe.

Wichtig ist dabei: `M1` und `M9` werden nicht mehr vom Programm fuer den
naechsten Durchlauf vorbereitet. Diese beiden Zellen sind bewusst in die Hand
des Benutzers gelegt, damit der Bewertungsblock kuerzer bleibt.

### Wie der Code in `M8` versteckt wird

Der Code wird nicht als lesbare vierstellige Zahl gespeichert, sondern
codiert. In `M8` liegt eine Zahl, bei der die Dezimalstelle `10^d` die Position
der Ziffer `d` im geheimen Code enthaelt.

Beispiel:

```text
Code 6413
```

Das bedeutet:

- Ziffer `6` steht an Position `1`  -> Beitrag `1 * 10^6`
- Ziffer `4` steht an Position `2`  -> Beitrag `2 * 10^4`
- Ziffer `1` steht an Position `3`  -> Beitrag `3 * 10^1`
- Ziffer `3` steht an Position `4`  -> Beitrag `4 * 10^3`

Daraus wird in `M8`:

```text
1024030
```

Die Ziffern `0`, `2` und `5` fehlen hier, daher stehen an ihren Stellen Nullen.
So kann das Programm spaeter fuer eine eingegebene Guess-Ziffer direkt die
passende Stelle aus `M8` auslesen.

Wenn man also `6413` in `M8` versteckt und als Guess `1234` eingibt, dann ist
die Bewertung:

- schwarz: `0`
- weiß: `3`
