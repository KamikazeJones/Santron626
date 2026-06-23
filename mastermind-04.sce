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
# M0 = geheimer CODE, gepackt
# M1 = aktueller GUESS-Aufbau fuer die Anzeige
# M4 = schwarze Treffer
# M5 = Stellenzaehler 3..0
# M6 = aktuelle CODE-Ziffer
# M7 = aktuelle GUESS-Ziffer
# M8 = Hilfsspeicher / 10^n / Arbeitsregister
# M9 = CODE Arbeitskopie
#
# Programmskelett:
# 1. Guess-Anzeige loeschen
# 2. Restarrays und Schwarzzaehler loeschen
# 3. Stellenzaehler auf 3 setzen
# 4. CODE-Ziffer extrahieren
# 5. R/S fuer Guess-Ziffer
# 6. Guess-Ziffer in M7 sichern
# 7. Guess-Anzeige neu aufbauen
# 8. Code- und Guess-Ziffer vergleichen
# 9. schwarz +1 oder beide Ziffern in Restarrays eintragen
# 10. Stellenzaehler erniedrigen
# 11. Schleife fuer alle 4 Positionen
# 12. Danach weisse Treffer ueber die beiden Restarrays bestimmen
#
# Noch offen:
# - Wie billig laesst sich der angezeigte Guess nach jeder Eingabe erweitern?
# - Reicht das Display selbst als Guess-Speicher, oder braucht M1 den Aufbau?
# - Wie viele Schritte kostet die Ausgabe waehrend der Eingabe wirklich?

:LOAD

# Initialisierung:
# nach dem Einschalten des Rechners ist M1 = 0

RCL 0 STO 9 # CODE nach Arbeitskopie kopieren 
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
    GOTO &black

    GOTO &next

%black
    1 M+ 4

%next
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

/run
goto 0 0
1 3 4 5 STO 0
0 STO 1 STO 4
3 STO 5
R/S
2 R/S
3 R/S
2 R/S
5 R/S

2 R/S
3 R/S
2 R/S
5 R/S

1 R/S
3 R/S
4 R/S
5 R/S
