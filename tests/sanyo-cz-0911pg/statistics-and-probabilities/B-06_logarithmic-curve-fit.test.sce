# Test: Logarithmic curve fit (B-6)
# DEG/RAD: Arbitrary
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-06_logarithmic-curve-fit.lst

/run
PS 2

0 STO 1
0 STO 2
0 STO 3
0 STO 4
0 STO 5
0 STO 6

GOTO 0 0
R/S
3 R/S
1 . 5 R/S
4 R/S
9 . 3 R/S
6 R/S
2 3 . 4 R/S
1 0 R/S
4 5 . 8 R/S
1 2 R/S
6 0 . 1 R/S
SKP
SKP
R/S
expect -47.02

RCL 7
expect 41.39
