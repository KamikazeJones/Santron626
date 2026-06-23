# Mastermind - Ansatz 16
#
# Wie Ansatz 15, aber ohne automatische Ausgabe von black/white.
#
# Der geheime Code wird vor dem Start codiert in M8 versteckt.
# Beispiel: 6413 -> 1024030
# Beispiel-Bewertung: Code 6413 gegen Guess 1234 ergibt 0 schwarz, 3 weiß.
# Vor dem ersten Lauf muss M9 auf 4 gesetzt werden.
#
# Nach der vierten GUESS-Ziffer haelt das Programm an:
# - schwarz = richtige Ziffer an richtiger Stelle
# - weiss   = richtige Ziffer an falscher Stelle
#
# RCL 1 zeigt die schwarzen Treffer.
# RCL 9 zeigt die weissen Treffer.
# Fuer den naechsten Guess setzt der Benutzer die Speicher zurueck mit:
# C/CE STO 1 4 STO 9
#
# Nach erneutem R/S setzt das Programm M0 und M2 wieder auf 1 und springt zur
# naechsten Guess-Eingabe. M1 und M9 setzt der Benutzer selbst zurueck.
#
# Vorbedingungen vor dem ersten Guess:
# - M1 = 0, black-Zaehler
# - M8 = codierter CODE
# - M9 = 4

:LOAD

# Konstanten vor jedem Guess initialisieren.
1 STO 0 STO 2
# Guess-Ziffer n lesen.
%guess
# M7 = Ziffer an Stelle 10^n aus M8.
1 0 STO 6
R/S
STO 4
RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7
M* 6 RCL 6 - RCL 6 = M- 7

# 39-Zellen-Bewertungsblock aus Ansatz 15, danach ein Reset von M2.
RCL 7 - +/- SKP RCL 0 M- 9
RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1
1 M+ 2
RCL 2 = SKP GOTO &guess

# Ergebnis-Halt. Danach darf der Benutzer M1/M9 ansehen und selbst setzen.
R/S
GOTO 0 0

list;

:RUN
save mastermind-16.lst;
