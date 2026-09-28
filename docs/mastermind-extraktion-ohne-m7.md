# Warum die neue Mastermind-Routine ohne RCL 7 auskommt

Die Verbesserung wurde in Mastermind 19 eingefuehrt und wird in
[Mastermind 20](../mastermind-20.sce) weiterverwendet. Entscheidend ist die
Umstellung der Extraktion: Sie liefert die gesuchte Codeposition direkt in X.
Die Bewertung bewahrt diese Position anschliessend in Y auf, waehrend X fuer
den Vorhandensein-Test verwendet wird.

## Was die Extraktion liefern muss

Der Geheimcode liegt codiert in M8. Fuer jede Ziffer d an Position p wird
p*10^d addiert. Beispiel fuer Code 6413:

```text
M8 = 1*10^6 + 2*10^4 + 3*10^1 + 4*10^3 = 1024030
```

Wird die Guess-Ziffer 4 eingegeben, muss die Extraktion p=2 liefern: Die 4
steht im Geheimcode an Position 2. Fuer eine fehlende Ziffer ist p=0.

## Warum frueher RCL 7 erforderlich war

Die Extraktion aus [Mastermind 17](../mastermind-17.sce) lautete:

```text
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7
```

Sie speicherte einen grossen Zwischenwert in M7 und zog davon am Ende einen
anderen Hilfswert ab. Erst durch `M- 7` entstand die gesuchte Position im
Speicher M7. X enthielt weiterhin den abgezogenen Hilfswert.

Fuer M8=1024030 und Guess-Ziffer 4 ergeben sich nach der alten Extraktion:

| Register | Inhalt |
| --- | ---: |
| X | 9000000100 |
| M7 | 2 |

Die Bewertung begann deshalb mit `RCL 7 - ...`. Das RCL war notwendig, um
die Position 2 aus M7 zurueckzuholen.

## Die neue Anordnung der Hilfsrechnung

Wir verwenden B=9000000000 und q=M8/10^d. Der Santron schneidet
Rechenergebnisse intern auf zehn signifikante Stellen ab. Dadurch entstehen:

```text
f = ganzzahliger Anteil von q
T = B + f
S = 2*B + 10*(ganzzahliger Anteil von f/10)

p = T - S + B
  = f - 10*(ganzzahliger Anteil von f/10)
```

Der letzte Ausdruck ist die Einerziffer von f und damit die gesuchte Position.
PS spielt fuer dieses Abschneiden keine Rolle: Es steuert nur die Anzeige.

Fuer das Beispiel ergibt sich:

```text
q = 102.403
T = 9000000102
S = 18000000100
p = 9000000102 - 18000000100 + 9000000000 = 2
```

Bei B+q verschwinden die Nachkommastellen, weil das Ergebnis zehnstellig ist.
Bei B+T verschwindet die Einerstelle, weil die Summe elfstellig ist und nur
zehn signifikante Stellen erhalten bleiben.

Die neue Tastenfolge ist:

```text
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 - M+ 4 RCL 4 + 9 EXP 9
-
```

Die Operationen arbeiten so zusammen:

1. `STO 4` speichert B in M4, waehrend die Addition q+B noch offen ist.
2. Das folgende `-` berechnet T und beginnt die Subtraktion T-X. T liegt
   jetzt sowohl in X als auch in Y.
3. **`M+ 4` bildet S im Speicher M4, waehrend T in X und Y erhalten bleibt.**
   Der Speicherbefehl schliesst die offene Subtraktion nicht ab.
4. `RCL 4` holt S nach X. Das folgende `+` berechnet T-S und beginnt die
   Addition des erneut eingegebenen B.
5. Das letzte `-` berechnet T-S+B. Die Position p liegt jetzt direkt in X.

Damit wird M7 fuer die Extraktion nicht mehr gebraucht.

## Wie die Bewertung p ohne erneutes Laden erhaelt

Das letzte `-` gehoert bereits zur Bewertung. Es schliesst die Extraktion
ab und beginnt gleichzeitig eine neue Subtraktion. Dabei wird p als linker
Operand in Y gespeichert. Ein separates abschliessendes `=` entfaellt.

Die aktuelle Bewertung aus Mastermind 20 beginnt anschliessend so:

```text
+/- SKP M- 1 M+ 9
RCL 2 * ...
```

X wird fuer den Vorhandensein-Test veraendert, aber Y bewahrt p auf:

| Zeitpunkt | X | Y | Offene Rechnung |
| --- | --- | --- | --- |
| nach dem letzten `-` der Extraktion | p | p | p-X |
| nach `+/-` | -p | p | p-X |
| nach dem Vorhandensein-Test | 0 oder 1 | p | p-X |
| nach `RCL 2` | aktuelle Guess-Position n | p | p-n |

Bei p=0 fuehrt `M- 1` nur das Abziehen von 0 aus und X bleibt 0. Bei p>0
ueberspringt SKP das M-; seine Registerziffer 1 wird als Zahl eingegeben.
`M+ 9` addiert diese 0 oder 1 zum Bewertungszaehler. Die offene Subtraktion
und der Wert p in Y bleiben dabei erhalten.

Das folgende `*` fuehrt p-n aus und beginnt sofort die Multiplikation fuer
den Schwarztest. Mit `+/- -` wird daraus -(p-n)^2: null bei gleicher Position,
negativ bei unterschiedlicher Position.

## Was dadurch gespart wird

Die Position wird direkt in X geliefert und fuer den naechsten Vergleich
in Y aufbewahrt. Das erneute Laden mit `RCL 7` entfaellt: Es haette zwei
Programmzellen benoetigt, eine fuer RCL und eine fuer die Registerziffer 7.
Zusaetzlich ersetzt das erste Minus der Bewertung ein abschliessendes `=`.
M7 steht als freier Speicher zur Verfuegung.

Der konkrete Vergleich der beiden Extraktionsfolgen wurde im JavaScript-
Emulator mit M8=1024030 und d=4 nachgerechnet: Die alte Folge liefert X=9000000100
und M7=2, die neue Folge X=2 und Y=2 bei bereits offener Subtraktion.
