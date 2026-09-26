# Mastermind 20 — automatische Ergebnisanzeige
#
# Vier verschiedene Geheimziffern und vier verschiedene Guess-Ziffern aus 1..8.
# Die Schleife laeuft weiterhin viermal: einmal pro Guess-Position.
# Vor jeder Eingabe zeigt das Display die Position 1, 2, 3 oder 4 an.
# Diese Positionsanzeige hat auch in Folgerunden keine Nachkommastelle.
# Nach der vierten Ziffer und R/S erscheint Schwarz.Weiss direkt im Display:
# 2.1 bedeutet zwei schwarze und einen weissen Treffer; 4.0 bedeutet geloest.
# Eine feste Nachkommastelle macht auch 0.0 und 4.0 eindeutig sichtbar.
#
# Vorbereitung: Nur den codierten Geheimcode in M8 speichern.
# Fuer jede Geheimziffer d an Position p wird p*10^d addiert.
# Beispiel 6413: 1*10^6 + 2*10^4 + 3*10^1 + 4*10^3 = 1024030.
# Daher: 1024030 STO 8. M8 wird vom Programm nicht veraendert.
# Beispiel mit 7 und 8: Code 8765 wird als 123400000 gespeichert:
# 1*10^8 + 2*10^7 + 3*10^6 + 4*10^5 = 123400000.
# Daher: 123400000 STO 8. Guess 5678 ergibt 0.4; Guess 8765 ergibt 4.0.
#
# Programm bei Zelle 00 starten. An jedem Eingabehalt eine Ziffer eingeben
# und R/S druecken. Beispiel Guess 1234: 1 R/S; 2 R/S; 3 R/S; 4 R/S.
# Fuer Geheimcode 6413 erscheint danach 0.3.
# R/S am Ergebnis-Halt startet die naechste Runde mit frischen Zaehlern.
# Fuer einen neuen Code vor diesem Neustart den codierten Wert in M8 speichern.
#
# Speicher: M2=Guess-Position, M4=Arbeitsspeicher, M8=Geheimcode.
# M9 startet bei 36 und enthaelt nach vier Ziffern 10*Schwarz+Weiss.
# M1 dient als Ziel fuer das Abziehen von 0; sein Wert bleibt erhalten.
# M0 und M7 werden nicht verwendet.
# M9 enthaelt damit eine andere Bewertung als in Mastermind 19.
# Das Programm belegt alle 72 Zellen. PS 0 und PS 1 aendern nur das Anzeigeformat.

:LOAD

# Rundenstart: PS 0 schaltet zur Anzeige ohne Nachkommastellen zurueck
# und beendet eine etwaige Zahleneingabe. Der Bewertungsvorrat ist 36.
PS 0
3 6 STO 9
1 STO 2

%guess
# M2 zeigt, welche Ziffer als Naechstes einzugeben ist. RCL beendet die
# Zahleneingabe, damit die getippte Guess-Ziffer den Positionswert ersetzt.
RCL 2 R/S

# Position p der Guess-Ziffer d aus M8 extrahieren.
# / und X<>Y bereiten M8/10^d vor. B=9*10^9 verwendet die interne
# Genauigkeit von zehn Stellen, um Nachkommastellen und hoehere Ziffern
# abzutrennen. M4 speichert erst B, dann die abgeschnittene Summe B+T.
# T-S+B liefert p; das folgende Minus schliesst diese Addition ab.
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 - M+ 4 RCL 4 + 9 EXP 9

# Vorhandene Ziffer: X=-p. Bei p=0 fuehrt M- 1 nur das Abziehen von 0 aus;
# X bleibt 0. Bei p>0 ueberspringt SKP das M-; die Registerziffer 1 wird
# als Zahl eingegeben. M+ 9 addiert deshalb 1 bei Vorhandensein, sonst 0.
- +/- SKP M- 1 M+ 9

# Schwarztest: -(p-n)^2 ist bei Schwarz null, sonst negativ.
# X<>Y rettet Y=4 fuer den spaeteren Schleifentest und setzt den Test nach X.
# Bei Schwarz bleibt X=0: das erste M- 9 zieht 0 ab und verbraucht seine 9.
# Andernfalls ueberspringt SKP das erste M-: dessen Registerziffer setzt X=9.
RCL 2 * +/- - 4 X<>Y SKP M- 9

# Das zweite M- 9 zieht bei Nicht-Schwarz 9 ab, bei Schwarz weiterhin 0.
# Pro Ziffer veraendert sich M9 netto um +1 (schwarz), -8 (weiss) oder
# -9 (fehlend). Weil es genau vier Ziffern sind, gilt am Ende:
# 36 + Schwarz - 8*Weiss - 9*Fehlend = 10*Schwarz + Weiss.
# Die offene Subtraktion mit Y=4 bleibt dabei fuer den Schleifentest erhalten.
M- 9
1 M+ 2
RCL 2 = SKP GOTO &guess

# Nach Ziffer 4 ist X=4-M2=-1. 10^X liefert deshalb direkt den Faktor 0.1.
# Multiplikation mit M9 macht aus 10*Schwarz+Weiss die Anzeige Schwarz.Weiss.
# PS 1 erzwingt eine Nachkommastelle; M9 behaelt den ganzzahligen Wert.
10^X * RCL 9 = PS 1
R/S
GOTO 0 0

list;
:RUN
save mastermind-20.lst;
