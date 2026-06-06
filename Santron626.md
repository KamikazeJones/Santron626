# Santron 626 Emulator - Stand

## Was wir gemacht haben

- Projektpfad auf `C:\Users\quass\Documents\Programmieren\Santron626` umgestellt.
- Programmspeicher als 72 Schritte mit Keycodes modelliert.
- `GTO` korrekt als dreiteilige Programmanweisung umgesetzt: `GTO` plus zwei Ziffern fuer die Zieladresse.
- `GTO nn` im RUN-Modus ergaenzt, um den Program Counter manuell zu setzen.
- `SKP(-)` als bedingtes Ueberspringen der folgenden `GTO`-Anweisung bei negativem X umgesetzt.
- `F` im LOAD-Modus korrigiert: `F` wird nicht gespeichert, sondern wartet auf die Folgetaste.
- Programmdateien als lesbares Listing-Format umgesetzt:
  `Zelle Keycode Zeichen`
- Speichern/Laden von Programmlistings eingebaut.
- Speichern nutzt, wenn verfuegbar, den Browser-Speichern-Dialog.
- Repository-Programme ueber `programs/manifest.json` und `.lst`-Dateien eingebaut.
- Programmlauf-Optionen eingebaut:
  - Schrittgeschwindigkeit
  - Zwischenergebnisse anzeigen
  - letzte Eingabe stehen lassen
  - Display abdunkeln
- Standard fuer Programmlauf auf `0 ms` und `Display abdunkeln` gesetzt.
- Turbo-Ausfuehrung fuer `0 ms` optimiert: mehrere Schritte pro Event-Loop-Tick.
- Rendering auf `requestAnimationFrame` gebuendelt.
- Display-Rendering im Modus `abdunkeln` und `letzte Eingabe` waehrend des Laufs eingefroren.
- Power-Rendering in eigene Funktion ausgelagert.
- Beim Ausschalten geht der Rechnerzustand verloren; Initialisierung passiert beim Einschalten.
- `PS` bzw. laut Handbuch `DP` umgesetzt:
  - `PS 0..7`: feste Nachkommastellen
  - `PS 8/9`: automatische Anzeige
  - Format bleibt bis zur Aenderung oder bis OFF erhalten
- Anzeigeformatierung fuer wissenschaftliche Schreibweise ueberarbeitet:
  - Mantissenvorzeichen
  - bis zu 8 Mantissenziffern
  - Exponentenvorzeichen
  - zweistelliger Exponent
  - Dezimalpunkt verbraucht keine eigene Displayposition
- `EXP` als Exponenteneingabe umgesetzt.
- `EX` bleibt `e^x`.
- `PI` intern auf 10 signifikante Stellen gesetzt: `3.141592654`.
- Anzeige zeigt im Auto-Modus 8 signifikante Stellen.
- Dezimalpunkt wird bei normalen Zahlen immer angezeigt; die `.`-Taste markiert nur den Beginn der Nachkommastellen.
- Interne Rechenergebnisse werden auf 10 signifikante Stellen gerundet.
- Klammern `(` und `)` eingebaut.
- Operator-Hierarchie zurueckgenommen: Der Rechner rechnet laut Handbuch nicht Punkt vor Strich.
- Grundoperatoren `+`, `-`, `x`, `÷` und `yx` schliessen die vorherige Rechnung wie `=` ab und setzen den naechsten Operator.
- `gamma.lst` korrigiert: fehlende schliessende Klammer vor der ersten Quadratwurzel ergaenzt.
- `=` schliesst offene Klammerregister nicht mehr pauschal; das ist fuer Programme mit Zwischenergebnissen in Klammern noetig.
- Speicheroperationen `STO`, `RCL`, `M+`, `M-`, `Mn×`, `Mn÷` weiterentwickelt.
- Layout mehrfach ueberarbeitet:
  - links Einstellungen
  - Mitte Rechner
  - rechts Programme und Programmspeicher
  - Panels koennen umbrechen
  - Rechner und Programmspeicher sollen bevorzugt nebeneinander bleiben
- Smartphone-Ansicht als horizontales Karussell umgesetzt:
  - Startansicht ist der Taschenrechner
  - Swipe nach rechts zeigt Display-Glow und Programmlauf
  - Swipe nach links zeigt Programme und Programmspeicher
- Mobile Zoom deaktiviert und Tasten/Controls auf schnelle Touch-Eingabe optimiert.
- Glow-Einstellungen links verbessert:
  - Ueberschrift `Display-Glow`
  - Labels `Radius` und `Intensitaet`
  - linkes Panel breiter
- Multiplikations- und Divisionsbeschriftungen optisch verbessert:
  - `×`
  - `Mn×`
  - `Mn÷`

## Wo wir stehen

Der Emulator ist inzwischen deutlich naeher am realen bzw. baugleichen Rechner:

- Die Programmlogik ist grundsaetzlich benutzbar.
- Programmlistings koennen gespeichert, geladen und aus dem Repository geoeffnet werden.
- Die Anzeige verhaelt sich wesentlich santron-typischer, inklusive DP/PS, EXP und wissenschaftlicher Darstellung.
- Die Rechenlogik folgt staerker der Registerlogik aus dem Handbuch: Grundoperatoren schliessen die vorherige Rechnung ab.
- Der Programmlauf ist performanter und hat konfigurierbares Anzeigeverhalten.
- Das Layout ist funktional in drei Bereiche getrennt.

## Noch offen / unsicher

- Layout bei allen Fensterbreiten weiter feinpruefen:
  - linkes Panel soll nie neben dem Rechner kleben
  - Rechner und Programmspeicher sollen bevorzugt nebeneinander bleiben
  - Umbruchverhalten braucht visuelle Kontrolle im Browser
- Falls moeglich weitere Handbuchstellen auswerten:
  - genaues Verhalten von wiederholtem `=`
  - genaue Regeln fuer sehr kleine/grosse Zahlen in der Anzeige
  - genaue Wirkung von `BST`/`SST` in allen Modi
  - Verhalten von `R/S` im laufenden Programm
  - ob Programme/Speicher beim echten OFF wirklich komplett verloren gehen
- Rechenmodell weiter mit Handbuchbeispielen vergleichen:
  - Rundung auf 10 Stellen
  - trigonometrische Ergebnisse
  - Exponentialdarstellung
  - Klammerregister/P-Register mit verschachtelten Klammern
  - Gamma-Programm als anspruchsvoller Programmlauf
- Repository-Programme erweitern:
  - Fakultät
  - Performance-Index-Testprogramm
  - kleine Demo-Programme
- Eventuell echte Tests/Szenarien als kleine Testliste dokumentieren:
  - `2 + 5 × 7 = 49` laut Handbuchlogik ohne Punkt-vor-Strich
  - `(2 + 3) × 4 = 20`
  - `1.23 EXP +/- 6`
  - `PI × 100`
  - `1 / 9.3326216 EXP 55`
- Alte Encoding-/Umlaut-Themen in Dateien pruefen; PowerShell-Ausgabe zeigte mehrfach Mojibake, die Browserdateien scheinen aber UTF-8 zu sein.
