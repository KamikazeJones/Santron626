# Test: Simultaneous equations in 2 unknowns (A-6)
# DEG/RAD: Arbitrary
# DPS: 2

load bin/sanyo-cz-0911pg/mathematics/A-06_simultaneous-equations-in-2-unknowns.lst

/run
PS 2

# a1 = 1, b1 = -1, c1 = -1, a2 = 1, b2 = 1, c2 = 3
1 STO 0
1 CHS STO 1
1 CHS STO 2
1 STO 3
1 STO 4
3 STO 5

GOTO 0 0
R/S
expect 1.00

R/S
expect 2.00
