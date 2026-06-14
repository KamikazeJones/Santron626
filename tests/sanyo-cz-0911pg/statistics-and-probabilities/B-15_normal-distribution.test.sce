# Test: Normal distribution (B-15)
# DPS: 4

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-15_normal-distribution.lst

/run
PS 4

0 STO 3

GOTO 0 0
R/S
2 . 0 R/S
expect 0.0228
