# Test: Complex arithmetic (+, -, X) (A-8)
# DEG/RAD: Arbitrary
# DPS: 0

load bin/sanyo-cz-0911pg/mathematics/A-08_complex-arithmetic-plus-minus-times.lst

/run
PS 0

# Preload M0, M1, M2, and M3 from the sheet notes
1 STO 0
2 STO 1
3 STO 2
4 STO 3

GOTO 0 0
R/S
expect 4.

R/S
expect 6.

R/S
expect -2.

R/S
expect -2.

R/S
expect -5.

R/S
expect 10.
