# 1 6 5 4 3 2 1
# -------------
# 1 3 0 1 0 4 2
# 1 3 0 0 1 2 4

:LOAD

0 STO 7
4 STO 5
%input
RCL 7
R/S +/- 10^X * RCL 5 = M+ 7  
1 M- 5
RCL 5 - 1 =
SKP
goto &input
RCL 7
R/S

5 STO 5
%loop
     1 0 M* 7 RCL 7
     + EXP 9 - EXP 9 -         # Vorderziffer abschneiden
                               # CODE-Ziffer merken
     M- 7                      # vorderste Ziffer entfernen
     ( 1 0 M* 8 RCL 8                   
     + EXP 9 - EXP 9 )         # Vorderziffer abschneiden
                               # CODE-Ziffer merken
     M- 8                      # vorderste Ziffer entfernen
     =
R/S     
#1 M- 5
#RCL 5 - 1 =
#SKP
#goto &loop

list;
save mastermind-06a.lst