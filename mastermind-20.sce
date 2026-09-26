# Mastermind 20 — automatische Ergebnisanzeige
#
# Vier verschiedene Geheimziffern und vier verschiedene Guess-Ziffern aus 1..6.
# Nach der vierten Ziffer und R/S erscheint Schwarz.Weiss direkt im Display:
# 2.1 bedeutet zwei schwarze und einen weissen Treffer; 4.0 bedeutet geloest.
# Eine feste Nachkommastelle macht auch 0.0 und 4.0 eindeutig sichtbar.
#
# Vorbereitung: Nur den codierten Geheimcode in M8 speichern.
# Fuer jede Geheimziffer d an Position p wird p*10^d addiert.
# Beispiel 6413: 1*10^6 + 2*10^4 + 3*10^1 + 4*10^3 = 1024030.
# Daher: 1024030 STO 8. M8 wird vom Programm nicht veraendert.
#
# Programm bei Zelle 00 starten. An jedem Eingabehalt eine Ziffer eingeben
# und R/S druecken. Beispiel Guess 1234: 1 R/S; 2 R/S; 3 R/S; 4 R/S.
# Fuer Geheimcode 6413 erscheint danach 0.3.
# R/S am Ergebnis-Halt startet die naechste Runde mit frischen Zaehlern.
# Fuer einen neuen Code vor diesem Neustart den codierten Wert in M8 speichern.
#
# Speicher: M0=1, M2=Guess-Position, M4=Arbeitsspeicher, M8=Geheimcode.
# M9 sammelt 10*Schwarz+Weiss. M1 und M7 werden nicht verwendet.
# M9 enthaelt damit eine andere Bewertung als in Mastermind 19.
# Das Programm belegt 70 Zellen; PS 1 aendert nur das Anzeigeformat.

:LOAD

# Rundenstart: Bewertung=0; Position und SKP-Konstante gemeinsam auf 1.
C/CE STO 9
1 STO 2 STO 0

%guess
R/S

# Position p der Guess-Ziffer d aus M8 extrahieren.
# / und X<>Y bereiten M8/10^d vor. B=9*10^9 verwendet die interne
# Genauigkeit von zehn Stellen, um Nachkommastellen und hoehere Ziffern
# abzutrennen. M4 speichert erst B, dann die abgeschnittene Summe B+T.
# T-S+B liefert p; das folgende Minus schliesst diese Addition ab.
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 - M+ 4 RCL 4 + 9 EXP 9

# Fehlende Ziffer: Bei p=0 laedt RCL 0 die 1, sonst setzt das uebrig
# gebliebene Registerargument 0 den Wert X=0. M9 verliert bei Fehlen 1.
- +/- SKP RCL 0 M- 9

# Schwarztest: -(p-n)^2 ist bei Schwarz null, sonst negativ.
# X<>Y rettet Y=4 fuer den spaeteren Schleifentest und setzt den Test nach X.
# SKP/RCL 0 erzeugt die Maske b=1 fuer Schwarz bzw. b=0 fuer Nicht-Schwarz.
RCL 2 * +/- - 4 X<>Y SKP RCL 0

# Der neue Trick: 10^b ist 10 fuer Schwarz und 1 fuer Nicht-Schwarz.
# M9 erhaelt damit pro Ziffer netto 10 (schwarz), 1 (weiss) oder 0 (fehlend).
# Bei Fehlen hebt die hier addierte 1 den vorherigen Abzug wieder auf.
# Die offene Subtraktion mit Y=4 bleibt dabei fuer den Schleifentest erhalten.
10^X M+ 9
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
