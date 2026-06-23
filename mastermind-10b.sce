# Mastermind - Ansatz 10b
#
# Alternative Trunkiermethode:
#   digit = floor(x / 10^n) - 10 * floor(x / 10^(n + 1))
#
# Vorbedingungen:
# - M8 = codierter Wert x
# - M4 = gesuchte Stelle n
#
# Ergebnis:
# - M7 = Ziffer an Stelle 10^n

:LOAD

RCL 8 / ( RCL 4 + 1 ) 10^X + EXP 9 - EXP 9 =
* 1 0 +/-
+ ( RCL 8 / RCL 4 10^X + EXP 9 - EXP 9 )
=
STO 7
R/S

list;

:RUN
save mastermind-10b.lst;
