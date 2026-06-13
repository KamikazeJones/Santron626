# Test: Determinant and inverse of a 2X2 matrix (A-9)
# DEG/RAD: Arbitrary
# DPS: 1

load bin/sanyo-cz-0911pg/mathematics/A-09_determinant-and-inverse-of-a-2x2-matrix.lst

/run
PS 1

# Preload M0..M3 from the sheet notes
1 STO 0
3 STO 1
2 STO 2
4 STO 3

GOTO 0 0
R/S
expect -2.0

R/S
expect -2.0

R/S
expect 1.5

R/S
expect 1.0

R/S
expect -0.5
