# Mastermind 19
#
# Mastermind fuer vier verschiedene Ziffern von 1 bis 6.
# Schwarz bedeutet: richtige Ziffer an richtiger Stelle.
# Weiss bedeutet: richtige Ziffer an falscher Stelle.
#
# Vorbereitung:
# - Der geheime Code wird codiert in M8 versteckt.
# - M0 wird vom Programm zu Beginn jeder Runde auf 1 gesetzt.
#   Eine manuelle Initialisierung mit 1 STO 0 ist nicht noetig.
#
# Codierung des geheimen Codes:
# Fuer jede Ziffer wird ihre Position als Stelle 10^Ziffer gespeichert.
# Beispiel: Code 6413
# - 6 steht an Position 1: 1 * 10^6 = 1000000
# - 4 steht an Position 2: 2 * 10^4 =   20000
# - 1 steht an Position 3: 3 * 10^1 =      30
# - 3 steht an Position 4: 4 * 10^3 =    4000
# - Summe:                            1024030
# Zum Verstecken druecken: 1024030 STO 8
#
# Spielablauf mit Code 6413 und Guess 1234:
# 1024030 STO 8
# Programm bei Schritt 00 starten.
# Das Programm haelt bei der ersten Guess-Ziffer an.
# 1 R/S; 2 R/S; 3 R/S; 4 R/S
# Danach haelt das Programm mit dem Ergebnis an.
# RCL 1 zeigt die schwarzen Treffer: 0
# RCL 9 zeigt die weissen Treffer: 3
#
# Naechster Guess:
# R/S druecken, dann die vier Guess-Ziffern wieder jeweils mit R/S eingeben.
# M0, M1, M2 und M9 initialisiert das Programm bei jeder Runde automatisch.
#
# Neues Spiel mit einem anderen Code:
# Den neuen codierten Wert in M8 speichern, dann nach einem Ergebnis R/S
# druecken. Das Programm startet die naechste Runde mit frischen Zaehlern.
#
# Das Programm belegt 67 Zellen. Der geheime Code bleibt in M8. Die
# Extraktion veraendert M4 als Arbeitsspeicher und liefert die Codeposition
# direkt in X; M7 wird nicht benoetigt.

:LOAD

# Rundenstart: Schwarz=0, SKP-Konstante M0=1, Weissvorrat=4, Position=1.
C/CE STO 1
1 STO 0
4 STO 9
1 STO 2

# Guess-Ziffer n lesen.
%guess
R/S

# Extraktion und Bewertung, direkt aus dem Displaywert d.
# X<>Y tauscht nach / RCL 8 die noch offenen Operanden, damit 10^X = 10^d.
# B=9*10^9 schneidet durch die zehnstellige interne Genauigkeit q=N/10^d ab:
# T=B+floor(q). M+4 veraendert M4 von B zu S=B+T, waehrend T in X bleibt.
# Die Rechnung T-S+B liefert die Ziffer an Stelle 10^d, also die Position p.
# M4 ist veraenderbarer Scratch; M8 bleibt der Geheimcode.
# Im Gesamtprogramm schliesst das erste Minus der Bewertung die Extraktion
# ab, deshalb steht vor der Bewertung kein separates Gleichheitszeichen.
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 - M+ 4 RCL 4 + 9 EXP 9

# X ist jetzt p. M9 wird um 1 fuer nicht vorhandene Ziffern und um 1 fuer
# schwarze Treffer vermindert; M1 zaehlt die schwarzen Treffer.
- +/- SKP RCL 0 M- 9
RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1
1 M+ 2
RCL 2 = SKP GOTO &guess

# Ergebnis-Halt. RCL 1 zeigt schwarz, RCL 9 zeigt weiss.
R/S
GOTO 0 0

list;

:RUN
save mastermind-19.lst;
