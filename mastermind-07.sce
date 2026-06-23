# Mastermind - Ansatz 07
#
# Kernidee:
# Die Codierung des geheimen CODE passiert nicht mehr im Programm.
# Der Operator versteckt den CODE bereits codiert in M8.
#
# Der Spieler gibt den GUESS Ziffer fuer Ziffer ein. Das Programm baut daraus
# keinen eigenen GUESS-Vektor mehr. Stattdessen wird jede eingegebene Ziffer
# sofort gegen den codierten CODE in M8 geprueft.
#
# Codierung:
# Es sind nur Ziffern 1..6 erlaubt.
# Fuer jede Ziffer d wird im CODE gespeichert, an welcher Position sie vorkommt.
# Die Stelle 10^d enthaelt also die Position dieser Ziffer.
#
# Beispiel:
# Wenn an zweiter Stelle eine 4 geraten wird, dann muss nur die CODE-Stelle
# 10^4 betrachtet werden:
#
# - CODE-Stelle 10^4 = 0: die 4 kommt im CODE nicht vor
# - CODE-Stelle 10^4 = 2: black
# - CODE-Stelle 10^4 != 0 und != 2: white
#
# Speicher:
# M8 = codierter CODE, wird vor Programmstart vom Operator gesetzt
# M5 = aktuelle Guess-Position
# M4 = black
# M9 = white
# M6 = aktuelle Guess-Ziffer
# M7 = CODE-Position der aktuellen Guess-Ziffer
#
# Bedienung:
# 1. Operator setzt M8 von Hand auf den codierten CODE.
# 2. Programm mit GOTO 0 0 starten.
# 3. Spieler gibt vier GUESS-Ziffern ein, jeweils mit R/S.
# 4. Jede Guess-Ziffer wird sofort bewertet.
#
# Auswertung pro Guess-Ziffer d an Position p:
#
# 1. Lies d ein.
# 2. Hole aus M8 die CODE-Position an Stelle 10^d.
# 3. Falls diese Position 0 ist: kein Treffer.
# 4. Falls diese Position p ist: black + 1.
# 5. Sonst: white + 1.
#
# Dezimalstelle holen:
# Die Ziffer an Stelle 10^n laesst sich berechnen als:
#
#   floor(x / 10^n) - 10 * floor(x / 10^(n + 1))
#
# Fuer x = 201034 und n = 3 ergibt das:
#
#   floor(201034 / 1000) - 10 * floor(201034 / 10000)
#   201 - 10 * 20 = 1
#
# Wichtig: Beide Quotienten muessen getrennt mit
#   + EXP 9 - EXP 9
# trunkiert werden. Ohne die erste Trunkierung heben sich die beiden Terme
# mathematisch fast weg bzw. liefern auf dem Rechner nicht die gewuenschte
# Dezimalstelle.
#
# Getestete Tastenfolge fuer x = 201034, n = 3:
#
#   201034 / 4 10^X + EXP 9 - EXP 9
#   * 10 +/-
#   + ( 201034 / 3 10^X + EXP 9 - EXP 9 )
#   =
#
# Ergebnis: 1.
#
# Vermutlich programmfreundlichere Variante:
#
#   q = floor(x / 10^n)
#   Ziffer = q - 10 * floor(q / 10)
#
# Dabei wird q in M7 zwischengespeichert:
#
#   RCL 8 / RCL 6 10^X + EXP 9 - EXP 9 = STO 7
#   / 10 + EXP 9 - EXP 9 = * 10 = M- 7
#   RCL 7
#
# Getestet mit M8 = 201034:
# - M6 = 1 -> M7 = 3
# - M6 = 3 -> M7 = 1
# - M6 = 5 -> M7 = 2

:LOAD

# Zaehler loeschen
0 STO 4
0 STO 9

# Positionszaehler, hier zunaechst 1..4 gedacht
1 STO 5

%input
    # Guess-Ziffer d lesen
    R/S
    STO 6

    # TODO:
    # M7 = CODE-Position an Stelle 10^d aus M8 holen.
    # Danach:
    # - M7 == 0  -> nichts zaehlen
    # - M7 == M5 -> black + 1
    # - sonst    -> white + 1

    # Naechste Position
    1 M+ 5
    RCL 5 - 5 =
    SKP
    GOTO &input

    # Vorlaeufige Anzeige: black.white
    RCL 4 + RCL 9 =
    R/S

list;
