# Test: Arithmetic mean (Non distribution) (B-1)
# DEG/RAD: Arbitrary
# DPS: 3

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-01_arithmetic-mean-non-distribution.lst

/run
PS 3

GOTO 0 0
R/S
9 . 1 4 0 R/S
9 . 1 4 5 R/S
9 . 1 6 5 R/S
9 . 1 8 5 R/S
9 . 1 9 0 R/S
SKP
R/S
expect 9.165
