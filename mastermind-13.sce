# Mastermind - Ansatz 13
#
# Wie Ansatz 12, aber die Bewertung wird direkt ausgegeben.
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
# - M9 = 4, wird erst um fehlende Ziffern und dann um black reduziert
#
# Nach der Bewertung:
# - M1 = black
# - M9 = white
#
# Hinweis:
# Diese kompakte Codierung setzt voraus, dass CODE und GUESS keine doppelten
# Ziffern enthalten. Ein doppelter GUESS wuerde sonst mehrfach gezaehlt.

:LOAD

# Guess-Ziffer n lesen.
R/S
STO 4

# M7 = Ziffer an Stelle 10^n aus M8.
1 0 STO 6
RCL 8 / RCL 4 10^X + EXP 0 9 + STO 7
M* 6 RCL 6 - RCL 6 = M- 7

# M9: fehlende Ziffern abziehen.
RCL 7 X^2 +/- SKP RCL 0 M- 9

# Black zaehlen. Dieselbe 0/1-Maske reduziert M9, dadurch wird M9 zu white.
RCL 7 - RCL 2 = X^2 +/- SKP RCL 0 M+ 1 M- 9

# Naechste Position.
1 M+ 2

# Fuer Position 1..3 zur Eingabe zurueck; nach Position 4 in die Ausgabe fallen.
4 - RCL 2 = SKP GOTO 0 0

# Ausgabe: black, dann white.
RCL 1
R/S
RCL 9
R/S

list;

:RUN
save mastermind-13.lst;
