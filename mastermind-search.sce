# Mastermind - Suchkopie
#
# Diese Datei ist absichtlich nur fuer Experimente gedacht.
# Die funktionierende Referenz bleibt mastermind-16.sce.
#
# Wenn eine neue Bewertungs- oder Extraktionsroutine getestet wird, wird sie hier
# eingesetzt. Die erzeugte Listing-Datei heisst mastermind-search.lst, damit
# mastermind-16.lst nicht versehentlich ueberschrieben wird.

:LOAD

1 STO 0 STO 2
%guess
R/S
STO 4
# 22-Zellen-Extraktion: Ergebnis in M7, M4 Scratch.
/ RCL 8 X<>Y 10^X + 9 EXP 9 STO 4 + STO 7 RCL 4 - RCL 4 = M- 7

RCL 7 - +/- SKP RCL 0 M- 9
RCL 2 * +/- - 4 X<>Y SKP RCL 0 M- 9 M+ 1
1 M+ 2
RCL 2 = SKP GOTO &guess

R/S
GOTO 0 0

list;

:RUN
save mastermind-search.lst;
