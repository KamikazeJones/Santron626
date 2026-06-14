# Test: Inverse normal integral (B-16)
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-16_inverse-normal-integral.lst

/run
PS 2

2 . 5 1 5 5 1 7 STO 0
0 . 8 0 2 8 5 3 STO 1
0 . 0 1 0 3 2 8 STO 2
1 . 4 3 2 7 8 8 STO 3
0 . 1 8 9 2 6 9 STO 4
0 . 0 0 1 3 0 8 STO 5

GOTO 0 0
R/S
0 . 0 0 1 3 4 9 9 R/S
expect 3.00
