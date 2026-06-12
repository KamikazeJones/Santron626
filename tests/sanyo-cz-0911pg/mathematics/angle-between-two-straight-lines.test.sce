# Test: Angle between two straight lines (A-4)
# DEG/RAD: DEG
# DPS: 3

load bin/sanyo-cz-0911pg/mathematics/angle-between-two-straight-lines.lst

/run
PS 3

# y1 = 1/2 x + 2, y2 = 5 - x
GOTO 0 0
0 . 5 R/S
1 CHS R/S
expect 71.565
