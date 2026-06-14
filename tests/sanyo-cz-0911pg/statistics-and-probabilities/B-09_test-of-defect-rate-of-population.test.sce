# Test: Test of defect rate of population (B-9)
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-09_test-of-defect-rate-of-population.lst

/run
PS 2

GOTO 0 0
R/S
5 0 R/S
2 0 0 R/S
expect 0.25
0 . 2 0 R/S
expect 1.63
