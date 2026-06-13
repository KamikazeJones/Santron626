# Fakultät
  /load

  * 1 STO
  %loop
  1
  - 2 = 
  SKP
  GOTO &body
  RCL 1
  R/S
  %body
  + 2 -
  M* 1
  GOTO &loop 

; save Fakultaet.lst
