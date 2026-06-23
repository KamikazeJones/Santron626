# Mastermind - Ansatz 08
#
# Versuch: Ziffer an Stelle 10^n durch wiederholtes Abziehen holen.
#
# Diese Datei testet nur die Extraktionsroutine. Die Testwerte werden vor dem
# Programmlauf in die Speicher gelegt, damit die Routine selbst in 72 Schritte
# passt.
#
# Vorbedingungen:
# - M8 = codierter Wert x, z.B. 201034
# - M4 = die zu extrahierende Stelle
#
# Ergebnis:
# - M7 = Ziffer an Stelle 10^n
#
# Beispiel fuer x = 201034 und n = 3:
# - M8 = 201034
# - M4 = 3
# Ergebnis: M7 = 1
#
# Stand:
# - Die Routine nutzt Programmschritte 00..49.
# - `10^M5` wird nicht mehr in M2 zwischengespeichert, sondern direkt in der
#   Subtraktion verwendet.
# - Wenn ein spaeteres Hauptprogramm M0 bereits dauerhaft auf 1 setzt, kann
#   `1 STO 0` entfallen.
# - Wenn M7 vor dem Aufruf garantiert 0 ist, kann auch `0 STO 7` entfallen.
#

:LOAD

1 STO 0
0 STO 7
6 STO 5
%loop
RCL 8 - RCL 5 10^X =
SKP
goto &subtract_ok
1 M- 5
RCL 5
SKP
goto &loop
RCL 7
R/S
%subtract_ok
STO 8
RCL 5 - RCL 4 = X^2 +/- SKP RCL 0 M+ 7
goto &loop 
list;

:RUN
save mastermind-08a.lst;
goto 0 0

2 0 1 0 3 4 STO 8 # Codierung
1 STO 4 # welche Stelle
R/S
