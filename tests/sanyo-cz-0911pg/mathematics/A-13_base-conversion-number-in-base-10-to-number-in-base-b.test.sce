# Test: Base conversion (Number in base 10 to number in base b) (A-13)
# DEG/RAD: Arbitrary
# DPS: 0

load bin/sanyo-cz-0911pg/mathematics/A-13_base-conversion-number-in-base-10-to-number-in-base-b.lst

/run
PS 0

GOTO 0 0
R/S
4 5 5 . 6 2 5 R/S
1 6 R/S
expect 7.

R/S
expect 12.

R/S
expect 1.

SKP
R/S
expect 10.
