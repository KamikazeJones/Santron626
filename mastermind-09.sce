# Mastermind - Ansatz 09
#
# Versuch: 08a als direkten Bewerter verwenden.
#
# Ergebnis dieses Versuchs:
# Ein einzelner GUESS-Schritt passt exakt in die 72 Programmschritte, wenn
# mehrere Werte vorher vorbereitet sind. Das Programm liest eine Guess-Ziffer,
# extrahiert aus dem codierten CODE die zugehoerige CODE-Position und addiert:
#
# - M9: hit = black + white, falls die Ziffer im CODE vorkommt
# - M1: black, falls die CODE-Position zur aktuellen Guess-Position passt
#
# White kann spaeter als M9 - M1 berechnet werden.
#
# Vorbedingungen vor dem Start:
# - M0 = 1
# - M1 = black-Zaehler
# - M9 = hit-Zaehler
# - M2 = aktuelle Guess-Position minus 1
# - M8 = codierter CODE
#
# Beispiel:
# CODE 1345 ist als 201034 codiert:
# - Ziffer 1 steht an Position 3 -> Stelle 10^1 enthaelt 3
# - Ziffer 3 steht an Position 1 -> Stelle 10^3 enthaelt 1
# - Ziffer 4 fehlt              -> Stelle 10^4 enthaelt 0
# - Ziffer 5 steht an Position 2 -> Stelle 10^5 enthaelt 2
#
# Fuer Position 1 gilt M2 = 0. Wird als Guess-Ziffer 3 eingegeben, dann ist das
# ein schwarzer Treffer: M9 += 1 und M1 += 1.
#
# Die Routine ist destruktiv nur fuer M6, nicht fuer M8.
# Sie belegt alle 72 Programmschritte. Fuer vier Guess-Ziffern, Positionsschleife
# und Endanzeige ist in dieser Form kein Platz mehr.

:LOAD

# Guess-Ziffer lesen und CODE kopieren.
R/S
STO 4
RCL 8
STO 6

# M7 zaehlt die gefundene CODE-Position minus 1.
# Dadurch ist M7 negativ, wenn die Ziffer fehlt.
1 +/-
STO 7

# Von 10^6 abwaerts alle Stellen abarbeiten.
6
STO 5

# Der erfolgreiche Subtraktionszweig steht vor der Schleife. Nach seiner Arbeit
# faellt er ohne extra GOTO direkt in die Schleife.
GOTO &loop

%subtract_ok
STO 6
RCL 5 - RCL 4 = X^2 +/- SKP RCL 0 M+ 7

%loop
RCL 6 - RCL 5 10^X =
SKP
GOTO &subtract_ok

1 M- 5
RCL 5
SKP
GOTO &loop

# M7 >= 0 bedeutet: Ziffer kommt im CODE vor.
RCL 7
SKP
RCL 0
M+ 9

# Black, wenn CODE-Position minus 1 gleich Guess-Position minus 1 ist.
RCL 7 - RCL 2 = X^2 +/- SKP RCL 0 M+ 1

list;

:RUN
save mastermind-09.lst;
