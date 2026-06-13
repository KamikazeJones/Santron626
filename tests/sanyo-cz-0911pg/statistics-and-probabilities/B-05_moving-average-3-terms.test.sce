# Test: Moving average (3-terms) (B-5)
# DEG/RAD: Arbitrary
# DPS: 3

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-05_moving-average-3-terms.lst

/run
PS 3

GOTO 0 0
R/S
3 . 4 R/S
3 . 3 R/S
3 . 5 R/S
expect 3.400

3 . 7 R/S
expect 3.500

3 . 6 R/S
expect 3.600

3 . 6 R/S
expect 3.633

3 . 9 R/S
expect 3.700

4 . 1 R/S
expect 3.867
