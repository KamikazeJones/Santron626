# Test: Base conversion (Number in base b to number in base 10) (A-12)
# DEG/RAD: Arbitrary
# DPS: 5

load bin/sanyo-cz-0911pg/mathematics/A-12_base-conversion-number-in-base-b-to-number-in-base-10.lst

/run
PS 5

2 STO 0

GOTO 0 0
1 R/S
0 R/S
1 R/S
0 R/S
1 =
expect 21.00000

GOTO 2 0
1 R/S
0 R/S
1 R/S
0 R/S
1 R/S
expect 0.65625
