# Mastermind - Ansatz 10
#
# Test: Ziffer an Stelle 10^n mit der Trunkiermethode holen.
#
# Formel:
#   q = floor(x / 10^n)
#   digit = q - 10 * floor(q / 10)
#
# Vorbedingungen:
# - M8 = codierter Wert x
# - M4 = gesuchte Stelle n
#
# Ergebnis:
# - M7 = Ziffer an Stelle 10^n

:LOAD

RCL 8 / RCL 4 10^X + EXP 9 - EXP 9 = STO 7
/ 1 0 + EXP 9 - EXP 9 = * 1 0 = M- 7
RCL 7
R/S

list;

:RUN
save mastermind-10.lst;
