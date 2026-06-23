# Mastermind - Ansatz 03
#
# Ziel dieses Versuchs:
# Die Extraktionsroutine aus mastermind.sce soll direkt als Programm geladen
# und dann mit positiven und negativen Testwerten aufgerufen werden.
#
# Beobachtungspunkt:
# Wenn M0 negativ ist, interessiert vor allem, ob die Routine als Vorderziffer
# noch 4 liefert oder ob ein negativer bzw. sonst verfremdeter Wert entsteht.

:LOAD

# Testprogramm:
# M0 = gepackter Testwert
# M5 = Stellenzaehler, anfangs 3
# M7 = extrahierte Vorderziffer
RCL 0
/ RCL 5 10^X
+ EXP 9 - EXP 9 -
STO 7
R/S

# Test 1: positiver Kontrollwert 4589
:RUN
C/CE
3 STO 5
4 5 8 9 STO 0
GOTO 0 0
R/S

# Test 2: negativer Wert -4589
C/CE
3 STO 5
4 5 8 9 +/- STO 0
GOTO 0 0
R/S

# Test 3: positiver Kontrollwert 4225
C/CE
3 STO 5
4 2 2 5 STO 0
GOTO 0 0
R/S

# Test 4: negativer Wert -4225
C/CE
3 STO 5
4 2 2 5 +/- STO 0
GOTO 0 0
R/S
