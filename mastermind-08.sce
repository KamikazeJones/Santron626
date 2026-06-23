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
# - M2 = aktuelle Zehnerpotenz, anfangs 10^6
# - M3 = Anzahl der hoeheren Stellen, also 6 - n
#
# Ergebnis:
# - M7 = Ziffer an Stelle 10^n
#
# Beispiel fuer x = 201034 und n = 3:
# - M8 = 201034
# - M2 = 1000000
# - M3 = 3
# Ergebnis: M7 = 1
#
# Getestete Ergebnisse fuer x = 201034:
# - n = 1, M3 = 5 -> M7 = 3
# - n = 3, M3 = 3 -> M7 = 1
# - n = 4, M3 = 2 -> M7 = 0
# - n = 5, M3 = 1 -> M7 = 2
# - n = 6, M3 = 0 -> M7 = 0
#
# Bewertung:
# Die Idee funktioniert, aber diese reine Extraktionsroutine belegt bereits
# 68 Programmschritte. Dabei sind M2 und M3 schon vorbereitet. Fuer das
# eigentliche Mastermind-Programm ist das in dieser Form zu teuer.
#
# Ausserdem ist die Routine destruktiv:
# - M8 wird auf den Rest unterhalb der Zielstelle reduziert
# - M2 endet als Zielpotenz 10^n
# Fuer mehrere Guess-Ziffern muesste M8 also kopiert oder neu aufgebaut werden.

:LOAD

# Wenn M3 == 0, direkt die aktuelle Stelle zaehlen.
RCL 3 - 1 =
SKP
GOTO &strip_power

%count_digit
    # Ziffer an aktueller Potenz M2 zaehlen.
    0 STO 7

%count_loop
    1 M+ 7
    RCL 2 M- 8
    RCL 8
    SKP
    GOTO &count_loop

    # Der letzte Abzug war einer zu viel.
    RCL 2 M+ 8
    1 M- 7
    RCL 7
    R/S

%strip_power
%strip_loop
    # Hoehere aktuelle Stelle entfernen.
    RCL 2 M- 8
    RCL 8
    SKP
    GOTO &strip_loop

    # Der letzte Abzug war einer zu viel.
    RCL 2 M+ 8

    # Zur naechstkleineren Zehnerpotenz wechseln.
    1 0 M/ 2
    1 M- 3

    # Noch weitere hoehere Stellen entfernen?
    RCL 3 - 1 =
    SKP
    GOTO &strip_power

    GOTO &count_digit

list;
