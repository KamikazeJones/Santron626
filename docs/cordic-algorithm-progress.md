# Fortschritt: CORDIC-Verständnismodell

Stand: 28. September 2026

## Ziel und Abgrenzung

`cordic-algorithm.js` wächst schrittweise zu einem einfachen Dezimalmodell, mit dem sich die Rechenschritte nachvollziehen lassen. Es ist derzeit **keine bitgenaue Nachbildung des MCS7529**. Die Konstanten aus dem rekonstruierten ROM sind vorhanden; die Rechenoperationen werden zunächst unabhängig davon als Verständnismodell aufgebaut.

## Bisher umgesetzt

- `DecimalNumber` speichert die Mantisse als Zeichenarray, dazu Mantissenvorzeichen, zweistelligen Exponenten als Zeichenarray und Exponentenvorzeichen. Der Dezimalpunkt ist implizit hinter der ersten Mantissenziffer. Automatische Normierung gibt es nicht.
- `copy()` erstellt eine unabhängige Kopie der Arrays und übernimmt Vorzeichen und Exponent.
- `digitadd(a, b, carry)` addiert zwei Ziffern über eine Lookup-Tabelle. `carry` ist 0 oder 1; das Ergebnis enthält Ziffer und Übertrag.
- `shiftRight(n)` stellt pro Schritt links eine Null voran, verwirft die letzte Mantissenziffer und erhöht den Exponenten. Der Stellenverlust ist beabsichtigt und sichtbar.
- `nineComplement()` bildet das Neunerkomplement der Mantissenziffern.
- `add(other)` gleicht die Exponenten an und addiert die Mantissen. Vorzeichen, Überträge und unterschiedliche Vorzeichen werden berücksichtigt. Die Methode ändert den Empfänger; `other` bleibt unverändert.
- `sub(other)` negiert eine Kopie des Subtrahenden und delegiert an `add`.

## Tests

`tools/test-cordic-algorithm.js` prüft `copy`, `nineComplement`, `shiftRight`, `digitadd`, `add` und `sub`, einschließlich Exponentenangleichung, Vorzeichen und Überträgen. Ausführen mit:

```sh
node tools/test-cordic-algorithm.js
```

Der gezielte Test wurde am 28. September 2026 nach der Ergänzung der Prüfungen zu `copy`, `nineComplement` und `shiftRight` erfolgreich ausgeführt.

## Geplante nächste Schritte

1. **Abgeschlossen:** `copy`, `nineComplement` und `shiftRight` mit Tests absichern.
2. Eine Mantissenverschiebung ergänzen, die den Exponenten unverändert lässt. Sie bildet die Multiplikation mit `10⁻ⁿ` für CORDIC-Schritte ab und ist von `shiftRight()` zur werterhaltenden Exponentenangleichung getrennt.
3. Einen einzelnen CORDIC-Vektorschritt mit alten x-/y-Werten, Mantissenverschiebung sowie `add`/`sub` implementieren und nachvollziehen.

## Offene Punkte

- `nineComplement()` ist vorhanden, wird aber derzeit nicht von `sub()` aufgerufen. `sub()` delegiert über Vorzeichenumkehr an `add()`; `add()` verwendet für unterschiedliche Vorzeichen die Hilfsfunktion `subtractDigits()` mit einer Neunerkomplement-Tabelle und initialem Übertrag 1. Eine spätere Bereinigung sollte klären, ob `nineComplement()` direkt in diesen Rechenweg eingebunden werden soll.
- `shiftRight()` und Exponentenangleichung schneiden Mantissenziffern ab; Rundung gibt es noch nicht.
- Das Zahlenformat und die Rechenregeln sind Modellannahmen. Registerbreiten, XC3-Arithmetik und Mikrocode des konkreten Santron626 sind damit nicht bitgenau abgebildet.
