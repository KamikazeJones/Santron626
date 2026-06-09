
# Programm eingeben
/load 

PS 5                    # Stelle um auf 5 Nachkommastellen 

+ PI y^x PI             # Addiere Pi und potenziere mit Pi
+ STO 0 EXP 9 - EXP 9   # Wert vor dem Komma ermitteln
- RCL 0 x<>y = r/s      # und von der Zahl abziehen
                        # Für die Zufallszahl z gilt:  1 > z >= 0

goto 0 2;               # und wieder von vorn                 

list;                   # Listing ausgeben

save x1.lst;            # Programm speichern nach x1.lst  

# Starte das Programm
/run
goto 0 0 
r/s{10}                 # 10 Werte erzeugen
