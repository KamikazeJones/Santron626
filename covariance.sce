

   /load
   
   # m0: sum(x*y)
   # m1: sum(X)
   # m2: sum(Y)
   # m3: sum(X^2)
   # m4: sum(Y^2)    
   # m5: n; Anzahl Wertpaare 
   
   # eingabe x * y
      
   m+ 2         # m2 += y  
   x^2 m+ 4     # m4 += y^2
  
   sqrt         # x * y  
   
   x<>y         # y * x

   m+ 1         # m1 += x
   x^2 m+ 3     # m3 += x^2
   
   sqrt         # y * x
   
   =            # x*y
      

   m+ 0         # m0 += x*y

   1 m+ 5       # m5 += 1; hier wird n hochgezählt

   r/s goto 0 0 # nächste Wert (x * y) eingeben oder skip
 
# -- Zeile 23 -- 
   r/s          # hier geht es weiter bei SKP
                # Eingabe ist RCL 3

   - (          # sum(X^2) -
   r/s          # Eingabe ist RCL 1
                # sum(X^2) - ( sum(X)^2
   / rcl 5 =    # sum(X^2) - ( sum(X)^2 / n ) 
   / ( 
   rcl 5 - 1 =  #  ( sum(X^2) - ( sum(X)^2 / n ) ) / (n - 1) 
   
   sqrt         # sqrt((sum(X^2) - (sum(X)^2 / n)) / (n-1)) 
   
   goto 2 3     # 
   
   r/s

# -- Zeile 44 --
   # m0: sum(x*y) = S(p)
   # m1: sum(X) = S(X)
   # m2: sum(Y) = S(Y)
   # m3: sum(X^2)
   # m4: sum(Y^2)    
   # m5: n; Anzahl Wertpaare 

   rcl 0        # S(p)
   - ( 
      rcl 1 *   # S(X) *  
      rcl 2     # S(Y) 
      /
      rcl 5     # S(p) - (S(X) * S(Y) / n
   =            
   / ( rcl 5 - 1 = # (S(p) - (S(X) * S(Y) / n)) / (n-1) 
                   
   
    
    
