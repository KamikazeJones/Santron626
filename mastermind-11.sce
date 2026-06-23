# Mastermind - Ansatz 11
#
# Bewertung einer Guess-Ziffer mit optimierter Trunkiermethode.
#
# Die Routine liest vier GUESS-Ziffern nacheinander per R/S, holt aus M8 die
# jeweilige CODE-Position und aktualisiert:
#
# - M9 = hit = black + white
# - M1 = black
#
# White kann spaeter als M9 - M1 berechnet werden.
#
# Vorbedingungen:
# - M0 = 1
# - M1 = black-Zaehler
# - M2 = aktuelle Guess-Position, anfangs 1
# - M8 = codierter CODE
# - M9 = 4, wird bei fehlender Ziffer erniedrigt und ist am Ende hit-Zaehler
#
# Codierungsregel:
# Die Stelle 10^d enthaelt die Position, an der Ziffer d im CODE steht.
# Fehlt eine Ziffer, steht an ihrer Stelle 0.
#
# Beispiel CODE 1264:
# - Ziffer 1 steht an Position 1 -> 1 * 10^1
# - Ziffer 2 steht an Position 2 -> 2 * 10^2
# - Ziffer 4 steht an Position 4 -> 4 * 10^4
# - Ziffer 6 steht an Position 3 -> 3 * 10^6
#
# Codiert ergibt das M8 = 3040210.
#
# Nach der vierten Ziffer haelt das Programm an. M1 und M9 enthalten dann:
# - M1 = black
# - M9 = black + white
# White ist M9 - M1.

:LOAD

# Guess-Ziffer n lesen.
R/S
STO 4

# M7 = Ziffer an Stelle 10^n aus M8.
RCL 8 / RCL 4 10^X + EXP 9 - EXP 9 = STO 7
/ 1 0 + EXP 9 - EXP 9 = * 1 0 = M- 7

# Hit zaehlen: M9 startet bei 4, bei M7 == 0 wird 1 abgezogen.
RCL 7 X^2 +/- SKP
RCL 0
M- 9

# Black zaehlen, wenn M7 == aktuelle Guess-Position M2.
RCL 7 - RCL 2 = X^2 +/- SKP RCL 0 M+ 1

# Naechste Position. Bei M2 < 5 wird das Abschluss-R/S uebersprungen und die
# naechste Guess-Ziffer gelesen. Bei M2 == 5 stoppt das Programm.
1 M+ 2
RCL 2 - 5 =
SKP
R/S
GOTO 0 0

list;

:RUN
save mastermind-11.lst;
