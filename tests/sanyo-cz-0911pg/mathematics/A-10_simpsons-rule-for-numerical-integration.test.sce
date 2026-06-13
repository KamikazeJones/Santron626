# Test: Simpson's rule for numerical integration (A-10)
# DEG/RAD: RAD
# DPS: 4

load bin/sanyo-cz-0911pg/mathematics/A-10_simpsons-rule-for-numerical-integration.lst

/run
PS 4

GOTO 0 0
R/S
0 . 3 9 2 6 9 9 1 R/S
0 R/S
0 R/S
0 . 1 4 6 4 R/S
0 . 5 R/S
0 . 8 5 3 6 R/S
1 R/S
0 . 8 5 3 6 R/S
0 . 5 R/S
0 . 1 4 6 4 R/S
RCL 1
expect 1.5708
