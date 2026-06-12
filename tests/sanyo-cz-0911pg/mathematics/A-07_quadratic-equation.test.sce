# Test: Quadratic equation (A-7)
# DEG/RAD: Arbitrary
# DPS: 2

load bin/sanyo-cz-0911pg/mathematics/A-07_quadratic-equation.lst

/run
PS 2

# a = 3, b = 2, c = -1
GOTO 0 0
R/S
3 R/S
2 R/S
1 +/- R/S
expect 0.33

R/S
expect -1.00
