# Mastermind - Ansatz 14
#
# Wie Ansatz 13, aber der Bewertungs-/Ausgabeblock wurde mit
# tools/stochastic-score-block.c um eine Zelle gekuerzt.
#
# Nach der vierten GUESS-Ziffer:
# - erster Halt: Anzeige = black
# - nach erneutem R/S: Anzeige = white
#
# Vorbedingungen:
# - M0 = 1
# - M1 = 0, black-Zaehler
# - M2 = 1, aktuelle Guess-Position
# - M8 = codierter CODE
# - M9 = 4

:LOAD

# Guess-Ziffer n lesen.
R/S
STO 4

# M7 = Ziffer an Stelle 10^n aus M8.
1 0 STO 6
RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7
M* 6 RCL 6 - RCL 6 = M- 7

# Optimierter Bewertungs-/Ausgabeblock.
RCL 7 = - +/- SKP RCL 0 M- 9
RCL 2 * - +/- SKP RCL 0 M- 9 M+ 1
RCL 0 M+ 2
4 X<>Y RCL 2 = SKP GOTO 0 0
RCL 1 R/S RCL 9 R/S

list;

:RUN
save mastermind-14.lst;
