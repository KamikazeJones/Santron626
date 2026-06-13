# Test: Numerical solution to differential equations (A-11)
# DEG/RAD: Arbitrary
# DPS: 7

load bin/sanyo-cz-0911pg/mathematics/A-11_numerical-solution-to-differential-equations.lst

/run
PS 7

GOTO 0 0
R/S
0 . 0 1 R/S
0 R/S
1 R/S
expect 1.0100500

R/S
expect 1.0202010
