# Mastermind - Ansatz 18
#
# Wie Ansatz 17, aber M1 und M9 werden vor jedem Guess automatisch
# initialisiert:
# - M1 = 0, schwarze Treffer
# - M9 = 4, Hit-Vorrat; am Ende bleiben die weissen Treffer uebrig
#
# Damit das noch in 72 Programmschritte passt, wird M0 nicht mehr vom Programm
# gesetzt. M0 muss vor dem Spiel einmal auf 1 gesetzt werden und bleibt danach
# unveraendert.
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
#
# Nach der vierten GUESS-Ziffer haelt das Programm an:
# - schwarz = richtige Ziffer an richtiger Stelle
# - weiss   = richtige Ziffer an falscher Stelle
#
# RCL 1 zeigt die schwarzen Treffer.
# RCL 9 zeigt die weissen Treffer.
# Danach reicht R/S fuer den naechsten Guess; M1, M2 und M9 werden vom Programm
# zurueckgesetzt.
#
# Vorbedingungen vor dem ersten Guess:
# - M0 = 1, interne Konstante
# - M8 = codierter CODE
#
# Einschraenkung:
# - CODE und GUESS verwenden nur die Ziffern 1 bis 6.
# - Die aktuelle Full-Test-Abdeckung prueft CODE und GUESS ohne doppelte Ziffern.

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
