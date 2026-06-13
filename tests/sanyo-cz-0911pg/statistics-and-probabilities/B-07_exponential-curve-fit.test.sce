# Test: Exponential curve fit (B-7)
# DEG/RAD: Arbitrary
# DPS: 5

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-07_exponential-curve-fit.lst

/run
PS 5

0 STO 0
0 STO 1
0 STO 2
0 STO 3
0 STO 4
0 STO 5

GOTO 0 0
R/S
1 R/S
1 R/S
2 R/S
2 R/S
SKP
SKP
R/S
expect 0.50000

RCL 6
expect 0.69315
