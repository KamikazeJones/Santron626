# Test: Coordinate translation and rotation (A-5)
# DEG/RAD: DEG
# DPS: 3

load bin/sanyo-cz-0911pg/mathematics/A-05_coordinate-translation-and-rotation.lst

/run
PS 3

# Preload M0, M1, M2 from the sheet notes
2 STO 0
1 STO 1
4 5 STO 2

# x0 = 2, y0 = 1, theta = 45, x = 4, y = 3
GOTO 0 0
4 R/S
3 R/S
expect 2.828

R/S
expect 0.000
