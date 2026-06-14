# Test: Binominal coefficient (nCr) (B-12)
# DPS: 0

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-12_binominal-coefficient-nCr.lst

/run
PS 0

GOTO 0 0
R/S
2 R/S
5 R/S
expect 10.
