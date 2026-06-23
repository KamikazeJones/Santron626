# Mastermind - Ansatz 04
#
# Idee:
# Der geheime CODE liegt bereits in M0.
# Der GUESS wird nicht als gepackte Zahl eingegeben und spaeter zerlegt,
# sondern waehrend des Programmlaufs Ziffer fuer Ziffer eingelesen.
#
# Vorteil:
# - Nur der CODE muss extrahiert werden.
# - Der GUESS ist beim Eintippen bereits isoliert.
# - Nach jeder Eingabe kann der bisherige GUESS wieder angezeigt werden.
#
# Speicherplan:
# M2 = geheimer CODE, gepackt
# M1 = aktueller GUESS-Aufbau fuer die Anzeige
# M4 = schwarze Treffer
# M5 = Stellenzaehler 3..0
# M9 = CODE Arbeitskopie
#
# Programmskelett:
# 1. Stellenzaehler auf 3 setzen
# 2. CODE-Ziffer extrahieren
# 3. Guess-Anzeige loeschen
# 4. R/S fuer Guess-Ziffer
# 5. Code- und Guess-Ziffer vergleichen
# 6. gleich? dann schwarz +1
# 7. Stellenzaehler erniedrigen
# 8. Schleife wiederholen fuer alle 4 Positionen
#

:LOAD

# Initialisierung:
# nach dem Einschalten des Rechners ist M1 = 0

RCL 2 STO 9 # CODE nach Arbeitskopie kopieren 
0 STO 4     # Zähler black zurücksetzt
3 STO 5     # Stellenzaehler n = 3

%loop
    RCL 9                    # CODE aus M9 laden
    / RCL 5 10^X             # durch 10^n teilen
    + EXP 9 - EXP 9 *        # Vorderziffer abschneiden
    STO 6                    # CODE-Ziffer merken
    RCL 5 10^X = M- 9        # vorderste Ziffer entfernen

    # GUESS fuer die Anzeige aufbauen:
    0 
    R/S
    # CODE-Ziffer mit GUESS-Ziffer vergleichen
    - RCL 6
    = X^2 +/- 
    SKP
    RCL 0
    M+ 4
    
    1 M- 5 RCL 5
    SKP
    GOTO &loop

%show
    RCL 4
    R/S
    goto 0 0
    
    # Danach:
    # - aus M2 und M3 die weissen Treffer bestimmen
    # - Ergebnis aus schwarz.weiss ausgeben
    # Dieser Teil ist hier noch nicht ausformuliert.

list;

save mastermind-black.lst;

/run
goto 0 0
1 3 4 5 STO 2
1 STO 0 # black increment SKP RCL 0 - Trick
0 STO 1 STO 4
3 STO 5
R/S
2 R/S
3 R/S
2 R/S
5 R/S

R/S
2 R/S
3 R/S
2 R/S
5 R/S

R/S
1 R/S
3 R/S
4 R/S
5 R/S
