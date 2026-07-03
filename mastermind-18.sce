# Mastermind 18
# Mastermind fuer vier verschiedene Ziffern von 1 bis 6.
# Schwarz bedeutet: richtige Ziffer an richtiger Stelle.
# Weiss bedeutet: richtige Ziffer an falscher Stelle.
#
# Vorbereitung:
# - M0 muss nach dem Laden des Programms einmalig auf 1 gesetzt werden:
#   1 STO 0
#   Danach bleibt M0 unveraendert; nicht pro Guess und nicht pro Code neu
#   setzen.
# - Der geheime Code wird codiert in M8 versteckt.
#
# Codierung des geheimen Codes:
# Fuer jede Ziffer wird ihre Position als Stelle 10^Ziffer gespeichert.
# Beispiel 1: Code 6413
# - 6 steht an Position 1: 1 * 10^6 = 1000000
# - 4 steht an Position 2: 2 * 10^4 =   20000
# - 1 steht an Position 3: 3 * 10^1 =      30
# - 3 steht an Position 4: 4 * 10^3 =    4000
# - Summe:                            1024030
# Zum Verstecken druecken: 1024030 STO 8
#
# Beispiel 2: Code 2561
# - 2 steht an Position 1: 1 * 10^2 =     100
# - 5 steht an Position 2: 2 * 10^5 =  200000
# - 6 steht an Position 3: 3 * 10^6 = 3000000
# - 1 steht an Position 4: 4 * 10^1 =      40
# - Summe:                            3200140
# Zum Verstecken des zweiten Codes M8 einfach ueberschreiben:
# 3200140 STO 8
# M0 bleibt 1.
#
# Spielablauf mit Code 6413 und Guess 1234:
# 1 STO 0
# 1024030 STO 8
# Programm bei Schritt 00 starten.
# Das Programm haelt bei der ersten Guess-Ziffer an.
# 1 R/S
# 2 R/S
# 3 R/S
# 4 R/S
# Danach haelt das Programm mit dem Ergebnis an.
# RCL 1 zeigt die schwarzen Treffer: 0
# RCL 9 zeigt die weissen Treffer: 3
#
# Naechster Guess:
# R/S druecken, dann die vier Guess-Ziffern wieder jeweils mit R/S eingeben.
# M1 wird dabei automatisch auf 0 gesetzt, M9 automatisch auf 4 und M2 auf 1.
# Anders als bei Mastermind 16 und 17 muessen diese Speicher nach einem Guess
# nicht mehr von Hand zurueckgesetzt werden.
#
# Neues Spiel mit dem zweiten Code:
# Wenn das Programm nach einem Ergebnis angehalten hat, 3200140 STO 8 druecken,
# dann R/S. Das Programm springt zu Schritt 00, setzt M1/M2/M9 zurueck und
# haelt bei der ersten Guess-Ziffer fuer den neuen Code an.

:LOAD

# Guess-Zaehler initialisieren: M1=0, M9=4, M2=1.
C/CE STO 1
4 STO 9
1 STO 2
# Guess-Ziffer n lesen.
%guess
R/S
STO 4
# M7 = Ziffer an Stelle 10^n aus M8. M4 ist Scratch.
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7

# Bewertung: M1 zaehlt schwarz, M9 wird von 4 auf weiss heruntergezaehlt.
RCL 7 - +/- SKP RCL 0 M- 9
RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1
1 M+ 2
RCL 2 = SKP GOTO &guess

# Ergebnis-Halt. Danach darf der Benutzer M1/M9 ansehen.
R/S
GOTO 0 0

list;

:RUN
save mastermind-18.lst;
