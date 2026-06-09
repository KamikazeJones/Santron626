# Santron 626 Emulator - Stand

## Was wir gemacht haben

- Projektpfad auf `C:\Users\quass\Documents\Programmieren\Santron626` umgestellt.
- Programmspeicher als 72 Schritte mit Keycodes modelliert.
- `GOTO` korrekt als dreiteilige Programmanweisung umgesetzt: `GOTO` plus zwei Ziffern fuer die Zieladresse.
- `GOTO nn` im RUN-Modus ergaenzt, um den Program Counter manuell zu setzen.
- `SKP(-)` als bedingtes Ueberspringen der folgenden `GOTO`-Anweisung bei negativem X umgesetzt.
- `F` im LOAD-Modus korrigiert: `F` wird nicht gespeichert, sondern wartet auf die Folgetaste.
- Programmdateien als lesbares Listing-Format umgesetzt:
  `Zelle Keycode Zeichen`
- Speichern/Laden von Programmlistings eingebaut.
- Speichern nutzt, wenn verfuegbar, den Browser-Speichern-Dialog.
- Repository-Programme ueber `programs/manifest.json` und `.lst`-Dateien eingebaut.
- Kommandozeilentool `santron-cli.js` zum Laden und Testen von Programmlistings eingebaut.
- Gemeinsamer Rechnerkern `santron-core.js` begonnen: DOM-freie Rechnerlogik. Das CLI nutzt diesen Core bereits.
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
- `E^X` steht fuer `e^x`, `10^X` fuer `10^x`.
- `PI` intern auf 10 signifikante Stellen gesetzt und hardwareartig trunkiert: `3.141592653`.
- Anzeige zeigt im Auto-Modus 8 signifikante Stellen.
- Dezimalpunkt wird bei normalen Zahlen immer angezeigt; die `.`-Taste markiert nur den Beginn der Nachkommastellen.
- Interne Rechenergebnisse werden auf 10 signifikante Stellen trunkiert.
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

- Programmlistings koennen fuehrende Kommentarzeilen mit `#` enthalten; diese werden im Programmpanel in einem separaten Kommentar-Tab angezeigt.

## Kommandozeilentool

`santron-cli.js` kann Programmlistings ohne Browser ausfuehren und eignet sich fuer Regressionstests.

Beispiele:

```powershell
node santron-cli.js --scenario "1 EXP 9 ="
node santron-cli.js --mode /DG /RUN /ON --scenario "1 + 2 =; expect 3."
node santron-cli.js --scenario "1 2 3 4 . 2 =; expect 1234.20" --dp 2
node santron-cli.js --program programs/square.lst --scenario "5 R/S; expect 25." --steps 20
node santron-cli.js --program programs/random-number-generator.lst --pc 1 --scenario "0 . 1 2 R/S; expect 0.1039" --steps 80 --dp 4
node santron-cli.js --scenario '1 $99 2 =; expect 12.'
node santron-cli.js --scenario '/LOAD X^2 R/S; save "mein-programm.lst"'
node santron-cli.js --scenario 'load "mein-programm.lst"; list'
node santron-cli.js --scenario '/CLR R/S; load "mein-programm.lst"; /RUN 5 R/S; expect 25.'
node santron-cli.js --scenario ':LOAD X^2 R/S GOTO 0 0; :RUN GOTO 0 0 2 R/S{4}'
node santron-cli.js --program programs/mastermind.lst --scenario "1 2 3 4 R/S; expect 1234.21; 5 6 2 1 R/S; expect 5621.12"
```

Wichtige Optionen:

- `--program <file>` laedt ein `.lst`-Programmlisting.
- `--scenario "<script>"` fuehrt ein Tastenszenario aus. Abschnitte werden mit `;` getrennt.
- Im Szenario koennen die Tastennamen aus dem Listing verwendet werden, z.B. `PI`, `Y^X`, `X<>Y`, `R/S`.
- Mit `{n}` kann ein Token wiederholt werden, z.B. `R/S{100}`.
- Mit `$nnn` kann ein Keycode direkt eingegeben werden, z.B. `$99` fuer Leerzeichen. In PowerShell dafuer einfache Anfuehrungszeichen verwenden.
- Schiebeschalter koennen als Tokens eingegeben werden: `/RD`, `/DG`, `/LOAD`, `/CLR`, `/RUN`, `/ON`, `/OFF`.
- Alternativ funktionieren die Schalter auch mit Doppelpunkt statt Slash: `:RD`, `:DG`, `:LOAD`, `:CLR`, `:RUN`, `:ON`, `:OFF`. Das ist besonders fuer Git Bash nuetzlich, weil Git Bash Slash-Argumente als Pfade umschreiben kann.
- `--mode` setzt den Anfangszustand der Schiebeschalter, z.B. `--mode /DG /RUN /ON`.
- `--steps <n>` begrenzt die Programmschritte.
- `--sst <n>` fuehrt einzelne Programmschritte aus.
- `--trace` zeigt jeden Programmschritt mit Displaywert.
- `--debug` zeigt vor jedem Programmschritt eine einzeilige Statusausgabe mit Display, x, y, pending, M0-M9, Winkelmodus, Laufmodus und Klammerstack.
- Im Szenario sind normale Abschnitte echte Tastendruecke. Wenn `R/S` den Programmlauf startet, laeuft das CLI automatisch bis zum naechsten programmierten Stop.
- Nach jedem `R/S` im RUN-Modus gibt das CLI den aktuellen Displaywert aus.
- `expect <anzeige>` prueft im Szenario die aktuelle Anzeige. Damit lassen sich mehrere Eingaben und Ausgaben in einem Programmlauf testen.
- `load "datei.lst"` laedt im Szenario ein Programmlisting.
- `save "datei.lst"` speichert im Szenario den aktuellen Programmspeicher als Listing.
- `list` gibt den aktuellen Programmspeicher dreispaltig aus.
- `--scenario-file <file>` liest das gleiche Szenarioformat aus einer Datei. `#` bis zum Zeilenende wird als Kommentar ignoriert, ausser innerhalb von Anfuehrungszeichen.

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
