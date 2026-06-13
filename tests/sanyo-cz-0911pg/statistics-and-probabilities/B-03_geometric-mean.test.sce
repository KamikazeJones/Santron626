# Test: Geometric mean (B-3)
# DEG/RAD: Arbitrary
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-03_geometric-mean.lst

/run
PS 2

GOTO 0 0
R/S
3 . 4 R/S
5 . 1 R/S
1 0 . 2 R/S
1 0 . 8 R/S
SKP
R/S
expect 6.61
