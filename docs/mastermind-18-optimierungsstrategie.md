# Strategie zur Verkuerzung von Mastermind 18

Stand: 27. September 2026. Ziel ist die Zahl belegter Programmzellen bei
gleichbleibender Spielbedienung. Die urspruenglichen Varianten wurden mit vier
verschiedenen Ziffern aus 1..6 entwickelt und geprueft. Mastermind 18 benoetigt M0=1 noch als
manuelle Vorbereitung. Mastermind 19 setzt M0 selbst und beginnt ohne diesen
Handgriff. Mastermind 20 zeigt nach der vierten Ziffer und R/S automatisch
Schwarz.Weiss mit einer Nachkommastelle an und erlaubt Ziffern aus 1..8.

## Ausgangspunkt und bereits gepruefte Kandidaten

`mastermind-18.lst` benutzt 71 Zellen, 00..70. Zelle 71 ist frei.

| Bestandteil | Zellen |
| --- | ---: |
| Initialisierung M1/M9/M2 | 9 |
| Eingabehalt | 1 |
| Vorbereitendes STO 4 | 2 |
| Extraktion | 22 |
| Bewertung und Positionszaehler | 26 |
| Schleifentest und Ruecksprung zur Eingabe | 7 |
| Ergebnishalt und Ruecksprung zum Rundenanfang | 4 |
| Gesamt | 71 |

Das vorbereitende `STO 4` in 10/11 wird vor der ersten Verwendung von M4
wieder ueberschrieben. `/` beendet die Zifferneingabe bereits. Die temporaere
Variante ohne dieses `STO 4` belegt 69 Zellen.

Eine weitere temporaere Variante belegt **65 Zellen**. Ausgangspunkt ist
folgende 21 Zellen lange Extraktion mit Ergebnis in X:

```text
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 - M+ 4 RCL 4 + 9 EXP 9 =
```

Sie liefert die gesuchte Position direkt in X. Deshalb entfaellt das erste
`RCL 7` der Bewertung. Im Gesamtprogramm entfaellt ausserdem ihr letztes `=`:
das erste `-` der Bewertung schliesst die Addition bereits ab. Die Extraktion
belegt dort also 20 Zellen. Die Bewertung beginnt unmittelbar mit:

```text
- +/- SKP RCL 0 M- 9
RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1
1 M+ 2
```

Die sechs gesparten Zellen bestehen aus zwei Zellen fuer `STO 4` vor der
Extraktion, einer Zelle innerhalb der neuen Extraktion, zwei Zellen fuer
`RCL 7` und einer Zelle fuer das entfallende abschliessende `=`.

M8 und M0 bleiben erhalten. M4 ist ein veraenderbarer Hilfsspeicher; M7 wird
nicht mehr als Ausgaberegister benoetigt. Die Schleife springt weiterhin nach
09. Die Referenzdateien `mastermind-18.sce` und `.lst` wurden nicht geaendert.

Pruefergebnisse fuer Original, 69-Zellen- und 65-Zellen-Variante:

- Jeweils PASS fuer 129.600 Code/Guess-Kombinationen in der C-VM, mit dem
  bestehenden `tools/test-mastermind-search.c` und `--m0-one`.
- Jeweils PASS fuer 1.080 Runden im JavaScript-Rechner: beliebige Anfangswerte
  der Hilfsspeicher, automatische Initialisierung, Ergebnisabfrage, Folgerunde
  und Codewechsel. Hier werden die Ergebniszaehler vor der Folgerunde nicht
  von Hand zurueckgesetzt.
- Fuer die eigenstaendige 21-Zellen-Extraktion: PASS fuer alle 2.160
  Kombinationen aus Geheimcode und eingegebener Ziffer im JavaScript-Rechner,
  inklusive Erhalt von M0/M1/M2/M8/M9.

Mastermind 19 uebernimmt die 65-Zellen-Variante. Die vorige Fassung setzte
M0 mit `1 STO 0` beim Rundenstart und belegte 68 Zellen; sie bestand den
129.600-Faelle-Test sowohl mit M0=0 als auch mit M0=1. Im JavaScript-Rechner
bestanden 1.080 Folgerunden, bei denen M0 vor jedem Neustart absichtlich auf
einen falschen Wert gesetzt wurde.

Die aktuelle Fassung initialisiert M2 und M0 gemeinsam mit `1 STO 2 STO 0`.
Der zweite STO speichert den unveraenderten X-Wert 1 in M0, so dass die
erneute Eingabe der 1 entfaellt. Das spart eine weitere Zelle; das Listing
belegt jetzt 67 Zellen. Die kombinierte STO-Folge wurde im Emulator einzeln
geprueft: M2=1 und M0=1, ohne offenen Speicherparameter. Das aktualisierte
Listing besteht ausserdem erneut den C-VM-Test fuer alle 129.600 Code/Guess-
Kombinationen, sowohl mit anfangs M0=0 als auch mit M0=1.

Die Kandidaten liegen waehrend dieser Sitzung unter
`/tmp/mastermind18-without-input-store.lst` und
`/tmp/mastermind18-extract-fused.lst`.

## Automatische Ergebnisanzeige in Mastermind 20

`mastermind-20.sce` benutzt einen gemeinsamen Bewertungszaehler in M9 und
belegt jetzt alle 72 Zellen. Vor jeder Eingabe zeigt `RCL 2` die Position
1, 2, 3 oder 4. `PS 0` am Rundenstart entfernt die Nachkommastelle auch bei
Folgerunden; `PS 1` am Ergebnis stellt die feste Nachkommastelle wieder her.

Fuer die Positionsanzeige wurde die Zaehlerrechnung verkuerzt. M9 startet bei
36 und enthaelt nach der vierten Ziffer `10*Schwarz+Weiss`. M0 wird nicht
verwendet. M1 dient ausschliesslich als Ziel fuer das Abziehen von 0; sein
Wert bleibt erhalten.

Code und Guess bestehen jeweils aus vier verschiedenen Ziffern von 1 bis 8.
Die Schleife laeuft weiterhin viermal, einmal pro Guess-Position. Die Ziffer
waehlt ueber `10^d` direkt ihre Stelle im codierten Geheimcode; die acht
moeglichen Ziffern werden nicht einzeln durchsucht.

Beispiel fuer Code 8765:

```text
1*10^8 + 2*10^7 + 3*10^6 + 4*10^5 = 123400000
Vorbereitung: 123400000 STO 8
Guess 5678: 5 R/S; 6 R/S; 7 R/S; 8 R/S -> 0.4
Guess 8765: 8 R/S; 7 R/S; 6 R/S; 5 R/S -> 4.0
```

Der groesste codierte Wert fuer Ziffern 1..8 ist 432100000. Er bleibt innerhalb
der internen zehnstelligen Genauigkeit. Auch die Addition mit B=9*10^9 in der
Extraktion bleibt zehnstellig: N/10^d ist fuer d>=1 hoechstens 43210000.

Der Vorhandensein-Test verwendet `SKP M- 1 M+ 9`: Bei p=0 zieht `M- 1`
nur 0 von M1 ab und X bleibt 0. Bei p>0 ueberspringt SKP das M-; seine
Registerziffer 1 wird als normale Zahl eingegeben. M9 erhaelt somit 1 bei
einer vorhandenen Ziffer und 0 bei einer fehlenden.

Der Schwarztest erzeugt weiterhin X=-(p-n)^2. `SKP M- 9 M- 9` zieht bei
Schwarz zweimal 0 ab. Andernfalls ueberspringt SKP das erste M-; die
Registerziffer 9 setzt X=9, und das zweite M- zieht diese 9 ab.

| Ziffer | Vorhandensein-Zuschlag | Nicht-Schwarz-Abzug | Netto in M9 |
| --- | ---: | ---: | ---: |
| fehlt im Code | 0 | -9 | -9 |
| vorhanden, falsche Position | 1 | -9 | -8 |
| richtige Position | 1 | 0 | 1 |

Mit S schwarzen, W weissen und F fehlenden Ziffern gilt S+W+F=4. Deshalb
ergibt der Startvorrat 36 nach vier Ziffern genau die kombinierte Bewertung:

```text
M9 = 36 + S - 8*W - 9*F
   = 36 + S - 8*W - 9*(4-S-W)
   = 10*S + W
```

Nach k Ziffern verbleibt noch der Vorrat `9*(4-k)` neben der bisherigen
Bewertung. Die Speicherung der Konstante M0=1 und die bisherige Umwandlung
der Schwarzmaske durch 10^X entfallen. Das schafft Platz fuer PS 0 und die
Positionsanzeige, waehrend die feste Ergebnisanzeige erhalten bleibt.

Am Schleifenende ist X=4-M2=-1. `10^X` liefert damit ohne neue Zahleneingabe
den Faktor 0.1. `10^X * RCL 9 = PS 1` zeigt unmittelbar Schwarz.Weiss an:
zum Beispiel 2.1 fuer zwei schwarze und einen weissen Treffer. PS 1 sorgt
fuer die feste Nachkommastelle, auch bei 0.0 und 4.0. M9 behaelt den
ganzzahligen kombinierten Wert. Ein R/S am Ergebnis startet die neue Runde.

Der JavaScript-Test `node tools/test-mastermind20.js` prueft alle 129.600
gueltigen Code/Guess-Kombinationen fuer Ziffern 1..6, die genaue Anzeige, Folgerunden mit
absichtlich veraenderten Arbeitsspeichern, die Positionsanzeige 1, 2, 3, 4
in jeder Runde, Codewechsel und den Erhalt von
M1/M7/M8. Er vergleicht auch das Web-Listing und das kommentierte Listing
mit der ausfuehrbaren Referenzdatei.

Fuer die Erweiterung auf 1..8 wurden zusaetzlich alle 53.760 einzelnen
Ziffernbewertungen geprueft: 1.680 Geheimcodes, je acht moegliche Guess-Ziffern
an vier Positionen. Die Beispielspiele 8765 gegen 5678 und 8765 wurden ebenfalls
geprueft. Ein vollstaendiger Test aller 2.822.400 Code/Guess-Kombinationen fuer
1..8 wurde bisher nicht durchgefuehrt.

## Weshalb die neue Extraktion funktioniert

Eine ausfuehrliche Erklaerung des Registerverlaufs und des entfallenden
`RCL 7` steht in [Extraktion ohne M7](mastermind-extraktion-ohne-m7.md).

Sei N der codierte Geheimcode und d die geratene Ziffer. Mit
`/ RCL 8 X<>Y 10^X` wird die Division N/10^d vorbereitet. Das folgende `+`
fuehrt sie aus. Fuer B = 9*10^9 werden dann zwei Zwischenwerte gebildet:

```text
q = N / 10^d
T = B + floor(q)
S = 2*B + 10*floor(floor(q)/10)
Ergebnis = T - S + B
         = floor(q) - 10*floor(floor(q)/10)
```

Die beiden floor-Schritte entstehen durch das Abschneiden auf zehn
signifikante Stellen: B+q ist zehnstellig, B+T ist elfstellig. Deshalb verliert
zuerst q seine Nachkommastellen und anschliessend die ganze Zahl ihre
Einerstelle. PS beeinflusst nur die Anzeige und ist daran nicht beteiligt.

Die neue Routine speichert B zunaechst in M4. `-` berechnet T und beginnt
bereits die Subtraktion T-S. `M+ 4` veraendert M4 zu S, waehrend T in X und
Y erhalten bleibt; die Speicheroperation schliesst die Subtraktion nicht ab.
RCL 4 holt S und das folgende `+` berechnet T-S. Anschliessend wird
T-S+B direkt in X berechnet. Beispiel N=1024030, d=4:

```text
q = 102.403
T = 9000000102
S = 18000000100
T-S+B = 2
```

## Reihenfolge der naechsten Versuche

1. **Extraktion mit Ergebnis in X optimieren.** Ein separates Target bekommt
   die neue 21-Zellen-Routine als Seed. Es fordert X=Position, schuetzt
   M0/M1/M2/M8/M9 und erlaubt beliebige Hilfsspeicher. Alle sechs Eingaben und
   die Ergebnisse 0..4 muessen vertreten sein. Ziel ist eine korrekte Routine
   mit hoechstens 20 Zellen. Ob sich diese Verkuerzung in der Integration
   zusaetzlich zum schon eingesparten `=` nutzen laesst, entscheidet die
   Laenge des Gesamtprogramms. Das alte M7-Target bleibt als Vergleich erhalten.

2. **Extraktion und Bewertung gemeinsam suchen.** Der Kandidat umfasst
   derzeit 44 Zellen: 20 Extraktion plus 24 Bewertung. Als Vertrag dienen
   die richtigen Aenderungen an M1/M9/M2 nach jeder Ziffer und die vier
   Eingabehalte. Ein bestimmtes Zwischenergebnis in M7 wird nicht gefordert.
   Dadurch duerfen X, Y und offene Rechenoperationen direkt weiterverwendet
   werden. Der erste Suchlauf laesst Initialisierung und Schleifenende fest;
   spaetere Laeufe beziehen den Schleifentest ein. Ein Kandidat mit 43 Zellen
   wuerde das Gesamtprogramm auf 64 Zellen verkuerzen.

3. **Weitere veraenderbare Hilfsspeicher untersuchen.** Die neue Routine ist
   bereits ein Beispiel: ein gespeicherter Hilfswert wird veraendert und
   erspart die Ausgabe ueber ein zweites Register. Weitere Varianten koennen
   eine Kopie von N oder einen Quotienten veraendern. Kopieren, Wiederherstellen
   und Initialisieren gehoeren immer zu den gezahlten Programmzellen. Ein
   bekannter Anfangswert eines Hilfsspeichers darf nur vorausgesetzt werden,
   wenn das Programm ihn zuvor herstellt.

4. **Andere Codierung als eigenen Algorithmus vergleichen.** Ein konkreter
   spaeterer Kandidat ist ein Code nach Position plus seine zwei fehlenden
   Ziffern a und b. Ein verkuerzbarer Arbeitscode liefert pro Position die
   passende Geheimziffer fuer Schwarz; `(d-a)*(d-b)=0` erkennt eine fehlende
   Guess-Ziffer fuer Weiss. Beispiel: fuer Code 6413 koennte der Arbeitscode
   3146 lauten; nacheinander werden die Einerstellen 6, 4, 1, 3 verarbeitet.
   Die fehlenden Ziffern sind a=2, b=5. Dazu sind nur vorhandene
   Rechenoperationen noetig.
   Die Rechnung, das Beschaffen von a/b und alle zusaetzlichen Speicher- und
   Kopierschritte muessen zuerst als vollstaendige Routine gezaehlt werden.
   Dieser Ansatz ist noch ungeprueft und hat niedrigere Prioritaet als 1/2.

## Suchwerkzeuge und Grenzen der bisherigen Pruefung

`tools/santron-search.c` mutiert flache Tastenfolgen mit einer stochastischen
Suche. Das passt zu den Rechenzustands-Tricks der vorhandenen Routine.
`tools/santron-tree-search.c` verwendet Population, Mutationen und Crossover
von Syntaxbaeumen. Er eignet sich eher fuer alternative Rechenformeln;
seine erzeugten Tastenfolgen muessen ebenfalls kuerzer sein.

Das bestehende `extract-search.target` fordert das Ergebnis in M7. Es
akzeptiert deshalb die neue X-Routine nicht. Auch `--promote-mastermind`
setzt den bisherigen M7-Vertrag voraus und baut intern eine aeltere
Programmstruktur mit anderer Initialisierung. Fuer die neuen Varianten
muss die Promotion an den neuen Vertrag und die echte Struktur von
Mastermind 18 angepasst werden; bis dahin werden Kandidaten separat in ein
vollstaendiges Listing eingesetzt und mit diesem Listing geprueft.

## Annahmekriterien fuer einen Fund

- Verglichen werden belegte Zellen des gesamten Programms, inklusive
  Initialisierung, Kopien, Schleife und Neustart. Viermal ausgefuehrte
  Schleifenschritte stehen trotzdem nur einmal im Programmspeicher.
- Ein kleines Trainings-Target fuehrt die Suche. Gefundene X-Extraktionen
  werden zusaetzlich fuer alle 360 Codes und sechs Ziffern, also 2.160
  Lookup-Faelle, geprueft.
- Ein zusammengebautes Programm muss die 129.600 gueltigen
  Code/Guess-Kombinationen bestehen. M8 und M0 muessen erhalten bleiben.
- Fuer Rundenwechsel und Aenderungen an der Initialisierung werden
  Folgerunden ohne manuelles Zuruecksetzen der Zaehler, Ergebnisabfragen und
  ein neuer Geheimcode geprueft. Hilfsspeicher duerfen keine versteckte
  Vorbelegung voraussetzen.
- Die Pruefung im JavaScript-Rechner bestaetigt die Integration in den
  tatsaechlichen 72-Zellen-Emulator. Beide Emulatoren bilden dieselbe interne
  Genauigkeit ab; diese Ergebnisse ersetzen keine Pruefung auf echter Hardware.

Ein erfolgloser Suchlauf beweist keine minimale Programm- oder
Extraktionslaenge. Bereits die 65-Zellen-Variante zeigt, dass der
Ausgabevertrag einer Teilroutine und ihre Verbindung zum folgenden Block
zusaetzliche Programmschritte kosten koennen.
