# Dezimales CORDIC für den MCS7529

`cordic.html` im Browser öffnen. Die Seite funktioniert auch direkt über `file://`, ohne Server, CDN oder zusätzliche Bibliotheken. Sie vergleicht Double mit einem BCD-Präzisionsmodell: Zerlegung, jede akzeptierte Rechenoperation, beide Vektoren, Näherungen, ihre Differenz und eine Math-Referenz. Schrittauswahl, Animation und JSON-Export sind verfügbar.

## Modul verwenden

Node/CommonJS:

```js
const { calculate, compare, ROM } = require('./cordic');
const trace = calculate('tan', 30, { unit: 'deg', levels: 10 });
console.log(trace.result); // ungefähr 0.577350268608
console.table(trace.steps);

const comparison = compare('tan', 30, { unit: 'deg', precision: 10 });
console.log(comparison.floating.result); // 0.5773502686076357
console.log(comparison.bcd.resultDecimal.text); // 5.773502659e-1
console.log(comparison.difference); // BCD − Double

const decimalTrace = calculate('ln', 7.3, { arithmetic: 'bcd', precision: 13 });
```

Im Browser zuerst `<script src="cordic-bcd.js"></script>`, dann `<script src="cordic.js"></script>` laden. Die globalen Objekte heißen `CordicBCD` und `Cordic`. Für ausschließlich Double genügt weiterhin `cordic.js`. Das Modul selbst benötigt kein DOM. Unterstützte Funktionen: `sin`, `cos`, `tan`, `atan`, `ln`, `log10`, `exp`, `pow10`.

- `input` muss eine endliche Zahl sein; Logarithmen benötigen positive Eingaben.
- `unit`: `rad` (Standard) oder `deg`; für `atan` bestimmt dies die Ausgabeeinheit.
- `levels`: ganze Zahl von 1 bis 10 (Standard 10). Stufe n verwendet 10⁻ⁿ.
- `arithmetic`: `double` (Standard) oder `bcd` für `calculate`.
- `precision`: 10 bis 16 Mantissenstellen im BCD-Modell (Standard 10). Die Oberfläche bietet 10, 13 und 16. Dies ist eine einstellbare Modellannahme, keine Wahl einer nachgewiesenen Chipregisterbreite.
- Exponentialfunktionen unterstützen einen natürlichen Exponenten von −708 bis 709. Unterlauf/Überlauf außerhalb dieses Modellbereichs wird abgefangen.
- Tangens an exakt erkannten Polstellen wird abgewiesen. Näherungen nahe Polstellen reagieren empfindlich auf die begrenzte Konstantengenauigkeit.
- Rückgabe: Ergebnis, Math-Referenz, absolute vorzeichenbehaftete Abweichung, Zähler pro Stufe und unveränderliche Zustandsschnappschüsse.
- `compare` liefert `floating`, `bcd`, `difference`, `rows`, `firstDifference` und `firstBranchDifference` (jeweils −1, wenn kein Unterschied gefunden wurde). `rows` ordnet Zustände nach Phase, Dezimalstufe und Wiederholung zu. Eine fehlende Seite steht auf `null`; aufeinanderfolgende Arrayindizes werden nicht blind verglichen.
- BCD-Zustände enthalten zusätzlich `decimal` mit exaktem Koeffizienten als String, Dezimalexponenten, Mantissenziffern und XC3-Nibbles. `resultDecimal` bewahrt den exakten Modell-Endwert. `losses` protokolliert Stellenverluste seit dem vorherigen Schnappschuss, einschließlich Eingaben, Konstanten, Zwischenoperationen und Prüfungen.
- Zahlen für Diagramme, `error` und `difference` werden an der Ausgabegrenze in Double umgewandelt. Das ändert die intern exakte BCD-Rechnung nicht. Für eine exakte Weiterverarbeitung die Dezimal-Snapshots verwenden.
- Zustand: `x`, `y`, `z`, `approximation`, Phase, Operation, Stufe, Dezimalverschiebung und ROM-Konstante. `z` ist je nach Funktion Restwinkel, akkumuliertes Ergebnis oder Restexponent. Während der Zerlegung entsteht noch kein neuer Funktionswert. Bei Sinus/Kosinus kennzeichnet `approximationKind: component` die unnormierte Komponente, erst `function` den ausgewerteten Funktionswert. Der Schritt `Auswertung` enthält `evaluationFactor`; er verändert x und y nicht. `approximationValid: false` kennzeichnet einen nicht definierten Tangens-Zwischenwert beim Vektorstart; er wird im Vergleich und Diagramm ausgelassen.

## Verfahren und Quellen

Die Konstanten sind aus Dunkelwinds Tabelle **MCS7529 014 ROM constants** übernommen; inklusive ihrer dort angegebenen letzten Stellen, statt sie pauschal durch gerundete oder abgeschnittene `Math.atan`-Werte zu ersetzen:

- [Chipanalyse, Registerschema, ROM-Konstanten und Emulator bei Richi’s Lab](https://www.richis-lab.de/MCS7529.htm)
- [Konstantentabelle als PDF](https://www.richis-lab.de/images/MCS7529/13.pdf)
- [Jacques Laporte: The Secret of the Algorithms](https://archived.hpcalc.org/laporte/TheSecretOfTheAlgorithms.htm)
- [Jacques Laporte: Trigonometry](https://archived.hpcalc.org/laporte/Trigonometry.htm)

Zusätzlich wurde das lokale `documents/mcs7529-emulator/code.js` untersucht: `cordic_rotate` bei Adresse 3B2 zerlegt den Winkel durch wiederholtes Subtrahieren; `winkel_passt` bei 3D4 erhöht den Zähler. Der Vektorpfad ab `cordic_loop` bei 278 verarbeitet Dezimalverschiebungen und Additionen/Subtraktionen. Die Zerlegung und Vektorrechnung sind deshalb als getrennte Phasen sichtbar.

Für trigonometrische Funktionen wird der reduzierte positive Winkel zuerst in eine Summe von `atan(10⁻ⁿ)` zerlegt. Pro gezähltem Schritt folgen, jeweils mit den **alten** x- und y-Werten:

```text
x′ = x − y · 10⁻ⁿ
y′ = y + x · 10⁻ⁿ
```

Der Vektor wächst dabei um √(1 + 10⁻²ⁿ). Für den Tangens fällt dieser Faktor im Quotienten weg; der Modus berechnet direkt y/x bzw. x/y. Bei Sinus und Kosinus bleibt die Komponente während sämtlicher Iterationen unnormiert. Erst danach folgt ein eigener `Auswertung`-Schritt: Der ausgewählte Funktionswert ergibt sich aus der vorzeichenkorrigierten Komponente geteilt durch √(x²+y²). **Der Arbeitsvektor wird dabei nicht normiert und bleibt auch im Ergebniszustand unverändert.** Dies ist eine mathematische Funktionsauswertung des Modells, keine geometrische Bewegung und kein belegter MCS7529-Mikroprogrammschritt. Für atan ist keine Vektornormierung nötig. Beim Arkustangens wird der Vektor in Gegenrichtung gedreht, solange y′ nicht negativ wird; die Winkel werden addiert.

Logarithmen zerlegen eine Mantisse durch wiederholte Division durch `(1 + 10⁻ⁿ)` und summieren die zugehörigen ROM-Logarithmen. Exponentialfunktionen zerlegen umgekehrt den Restexponenten in diese Logarithmen und bauen das Produkt durch `x ← x + x·10⁻ⁿ` auf. Potenzen von 10 übernehmen die Bereichsreduktion.

## Was das BCD-Modell berechnet

`cordic-bcd.js` verwendet BigInt für die Dezimalarithmetik, nicht ein nachträgliches `toFixed` auf Double-Ergebnissen. Ein Wert ist ein vorzeichenbehafteter Koeffizient mit der gewählten Anzahl Mantissenziffern und einem separaten Dezimalexponenten. Die Rechnung erfolgt ohne binäre Gleitkomma-Zwischenwerte.

- Addition und Subtraktion richten die Exponenten exakt aus. Das Ergebnis wird auf die Mantissenbreite abgeschnitten, **gegen null**, nicht gerundet.
- Multiplikation berechnet das exakte Integerprodukt und schneidet ab. Division verwendet eine Integerdivision mit zusätzlichen Stellen; der Rest wird protokolliert. Die Quadratwurzel verwendet eine ganzzahlige Wurzelsuche und schneidet ebenfalls ab.
- Eine Dezimalverschiebung innerhalb der Mantisse verliert die herausgeschobenen Ziffern **vor** der anschließenden Addition/Subtraktion. Sie ist deshalb bewusst eine andere Operation als das bloße Ändern des Exponenten. Beispiel bei 10 Stellen: `1.234567891` um drei Stellen verschoben ergibt `0.001234567`, während reine Exponentskalierung `0.001234567891` erhalten würde.
- XC3 wird für die Darstellung der Mantissenziffern erzeugt: Ziffer 0 → Nibble 3, Ziffer 9 → Nibble C. Vorzeichen und Exponent liegen im Modell separat. Das ist kein Abbild des physischen Registerinhalts.
- Numerische Eingaben werden über ihre kürzeste JavaScript-Dezimaldarstellung eingelesen, etwa `0.1` als exakt dezimale `0.1`. Bei extremen/subnormalen Zahlen kann schon dies vom binären Number-Wert abweichen. Die Math-Referenz bezieht sich auf den ursprünglichen Number-Wert.

Beide Modi verwenden dieselben CORDIC-Formeln und ROM-Konstanten; Bereichsreduktion und Auswertung werden ebenfalls mit dem jeweiligen Arithmetikbackend gerechnet. Die Math-Referenz bleibt unabhängig. Die Grafik verwendet die tatsächlichen unnormierten x-/y-Werte auf einer für den gesamten Ablauf festen Koordinatenskala. Der Einheitskreis dient nur als Referenz. Teilstrecken zeigen Δx und Δy. Auch nach der Funktionsauswertung bleibt der Vektor außerhalb des Einheitskreises stehen. Das Konvergenzdiagramm zeigt für Sinus und Kosinus zuerst die unnormierte Komponente und anschließend den daraus ausgewerteten Funktionswert; es zeigt keine Verkürzung des Arbeitsvektors.

Beispiele mit zehn Mantissenstellen und zehn Dezimalstufen:

| Rechnung | Double | BCD-Modell | Unterschied BCD − Double |
| --- | --- | --- | --- |
| tan(30°) | 0.5773502686076357 | 0.5773502659 | ca. −2.71 × 10⁻⁹ |
| ln(7.3) | 1.987874347605505 | 1.987874289 | ca. −5.86 × 10⁻⁸ |

Bei `tan(30°)` sind die Zähler gleich. Bei `ln(7.3)` akzeptiert das BCD-Modell auf den letzten beiden Stufen weniger Divisionen: n=8 zählt 5 statt 8, n=9 zählt 2 statt 3. Der Slider und das Schrittprotokoll markieren die einseitigen Operationen. Für die Vektorgrafik wird die inaktive Seite auf ihrem letzten Zustand gehalten; die Vergleichstabelle erfindet dafür keinen Rechenschritt.

## Genauigkeitsgrenze

Dies ist weiterhin eine **Rekonstruktion auf Algorithmusebene**, keine bitgenaue Chipemulation. Das neue Backend zeigt die Wirkung begrenzter Dezimalpräzision und von Stellenverlusten, aber noch keine nachgewiesenen MCS7529-Endwerte.

Das Registerschema zeigt 13 Ziffernpositionen plus ein weiteres Nibble bei R2/R3/RC/RD. Daraus folgen **nicht** 13 Mantissenstellen: Das Disassemblat verwendet Teilmasken für Mantisse, Exponent und Steuerwerte. Ihre konkreten Breiten und wechselnden Belegungen, serielle ALU-Takte, Carry-/Borrow-Signale und ROM-Befehlsfolgen bildet das Modell nicht ab. Addition mit exakter Exponentenausrichtung, automatische Normalisierung und ein separater beliebig großer Exponent sind ebenfalls Modellregeln, keine Behauptungen über den Chip. Eine vollständige Chipnachbildung müsste diese Regeln durch die konkreten Registeroperationen des Mikroprogramms ersetzen und die Zustände gegen den Referenzemulator prüfen.

Die belegten Konstanten und Firmware gehören zur Variante **014**. Eine identische Firmware in der Santron-Maskenvariante ist damit nicht nachgewiesen. Weniger Dezimalstufen zeigen den Effekt früher Abbrüche, ohne eine historische Präzisionseinstellung zu behaupten.

## Prüfen

```sh
node tools/test-cordic.js
node tools/test-cordic-bcd.js
npm test
# Falls Playwrights Chromium installiert ist:
node tools/test-cordic-gui.mjs
```

Die numerischen Tests vergleichen Wertebereiche, Quadranten, Extremwerte und Definitionsbereiche und prüfen Vektorlängen-, Logarithmus- und Konvergenzinvarianten. Die BCD-Tests prüfen Dezimaloperationen, Abschneiden, Verschiebungsverluste, XC3-Ziffern und den Vergleich bei drei Präzisionen; unabhängige Integer-Ungleichungen prüfen die Divisionsgenauigkeit. Der GUI-Test prüft beide Arithmetiken, einseitige Schritte, Präzisionswahl, Schrittauswahl, Animation, Fehlerbehandlung, alle Funktionen, Export und ein schmales Viewport.
