# Mastermind - Ansatz 05
#
# Neue Vereinfachung:
# Der geheime CODE liegt bereits positionsweise in den Speichern.
# Damit entfaellt die CODE-Extraktion vollstaendig.
#
# Annahme:
# M1 = CODE-Ziffer 1
# M2 = CODE-Ziffer 2
# M3 = CODE-Ziffer 3
# M4 = CODE-Ziffer 4
#
# Idee:
# Der GUESS wird waehrend des Programmlaufs Ziffer fuer Ziffer eingelesen.
# Fuer jede Position wird die passende CODE-Ziffer per RCL geladen und direkt
# mit der aktuellen Guess-Ziffer verglichen.
#
# Vorteil gegenueber Ansatz 04:
# - Keine Zerlegung des CODE mehr
# - Keine Arbeitskopie des CODE
# - Schleife bleibt erhalten
#
# Noch offen:
# - Wo werden Guess-Aufbau, Restarrays und Schwarzzaehler am besten abgelegt,
#   ohne mit M0..M3 zu kollidieren?
# - Lohnt sich eine Schleife mit Zaehler wirklich noch, wenn es keine indirekte
#   Adressierung gibt?
# - Vielleicht ist jetzt entrollter Code fuer vier Positionen doch billiger.
#
# Moegliches Speicherlayout:
# M1..M4 = CODE-Ziffern 1..4
# M5     = schwarze Treffer
#
# Programmskelett ohne CODE-Extraktion:
#
:LOAD

1 STO 0              # Decrement vorbereiten
0 STO 5              # schwarze Treffer loeschen

4 STO 7
%loop
1 M- 7
RCL 1 STO 9 RCL 7 - 3 = SKP goto &compare
RCL 2 STO 9 RCL 7 - 2 = SKP goto &compare
RCL 3 STO 9 RCL 7 - 1 = SKP goto &compare
RCL 4 STO 9 RCL 7 + 0 = SKP goto &compare

RCL 5
R/S
GOTO 0 0

%compare
R/S - RCL 9 = X^2 +/- SKP RCL 0 M+ 4 # Guess-Ziffer 1 eingeben
goto &loop
list;

:RUN

GOTO 0 0
1 STO 1 
3 STO 2 
5 STO 3
2 STO 4
R/S
4 R/S
3 R/S
1 R/S
2 R/S

#
# Danach:
# - weisse Treffer aus M7 und M8 berechnen
# - Anzeige aus Guess und schwarz.weiss aufbauen
#
# Erwartung:
# Dieser Ansatz ist nur dann wirklich besser, wenn das direkte Laden von
# M0..M3 billiger ist als jede Form von CODE-Extraktion. Das ist hier die
# eigentliche Arbeitsfrage.

