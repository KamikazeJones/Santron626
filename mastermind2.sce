# Programm eingeben

goto 0 0
 
/LOAD

# einlesen des CODE
R/S
STO 0
STO 7

# ab jetzt versucht der User den Code zu finden
R/S # GUESS Eingabe
STO 1
STO 8

# Bewertung des GUESS
# ca: CODE-Array
EXP 7 STO 2
# ga: GUESS-Array 
STO 3
# sw: schwarz/weiß Bewertung s.w
0 STO 4

# N: Schleifenzähler
3 STO 5 

%loop
    RCL 0 # lade CODE
    / EXP RCL 5
    + EXP 9 - EXP 9 - STO 6 # first CODE

    ( RCL 1 # lade CODE
    / EXP RCL 5
    + EXP 9 - EXP 9 ) STO 7 # first GUESS
    
    = X^2 +/- # < 0 wenn ungleich
    
    SKP 
    GOTO &black
    
    EXP RCL 6 M+ 2 # ca
    EXP RCL 7 M+ 3 # ga
    GOTO %repeat
%black
    1 M+ 4

%repeat
    RCL 6 * EXP RCL 5 = M- 0
    RCL 7 * EXP RCL 5 = M- 1
    1 M- 5
    SKP
    GOTO &loop
    
# jetzt die weiße Bewertung
;list;
