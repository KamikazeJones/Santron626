# Test: Estimation of population mean (95% confidence interval) (B-8)
# DEG/RAD: Arbitrary
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-08_estimation-of-population-mean-95-percent-confidence-interval.lst

/run
PS 2

GOTO 0 0
R/S
1 5 8 . 5 R/S
6 . 5 R/S
1 0 0 R/S
expect 157.20

R/S
expect 159.80
