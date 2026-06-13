# Test: Mean, Variance, Standard Deviation (B-4)
# DEG/RAD: Arbitrary
# DPS: 6

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-04_mean-variance-standard-deviation.lst

/run
PS 6

0 STO 1
0 STO 2
0 STO 3

GOTO 0 0
R/S
9 . 1 4 0 R/S
9 . 1 4 5 R/S
9 . 1 6 5 R/S
9 . 1 8 5 R/S
9 . 1 9 0 R/S
SKP
R/S
expect 0.020248

RCL 4
expect 9.165000

RCL 5
expect 0.000410
