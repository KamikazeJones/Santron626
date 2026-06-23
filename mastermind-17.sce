# Mastermind - Ansatz 17
#
# Wie Ansatz 16, aber mit der 22-Zellen-Extraktionsroutine aus der Suche.
#
# Der geheime Code wird vor dem Start codiert in M8 versteckt.
# Beispiel: 6413 -> 1024030
#
# Codierung:
# - Ziffer 6 an Position 1: 1 * 10^6 = 1000000
# - Ziffer 4 an Position 2: 2 * 10^4 =   20000
# - Ziffer 1 an Position 3: 3 * 10^1 =      30
# - Ziffer 3 an Position 4: 4 * 10^3 =    4000
# - Summe:                            1024030
#
# Beispiel-Bewertung: Code 6413 gegen Guess 1234 ergibt 0 schwarz, 3 weiss.
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
# - M1 = 0, Schwarz-Zaehler
# - M8 = codierter CODE
# - M9 = 4
#
# Einschraenkung:
# - CODE und GUESS verwenden nur die Ziffern 1 bis 6.
# - Die aktuelle Full-Test-Abdeckung prueft CODE und GUESS ohne doppelte Ziffern.

:LOAD

# Konstanten vor jedem Guess initialisieren.
1 STO 0 STO 2
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

# Ergebnis-Halt. Danach darf der Benutzer M1/M9 ansehen und selbst setzen.
R/S
GOTO 0 0

list;

:RUN
save mastermind-17.lst;
