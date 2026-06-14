# Test: Chi-square evaluation (B-14)
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-14_chi-square-evaluation.lst

/run
PS 2

0 STO 2

GOTO 0 0
R/S
1 1 5 R/S
1 0 0 R/S
8 5 R/S
1 0 0 R/S
SKP
SKP
R/S
expect 4.50
