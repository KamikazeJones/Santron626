# Test: Arithmetic mean (Frequency distribution) (B-2)
# DEG/RAD: Arbitrary
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-02_arithmetic-mean-frequency-distribution.lst

/run
PS 2

GOTO 0 0
R/S
0 . 5 R/S
2 . 5 R/S
3 R/S
6 R/S
8 R/S
5 R/S
2 R/S
1 R/S
SKP
R/S
expect 3.50
