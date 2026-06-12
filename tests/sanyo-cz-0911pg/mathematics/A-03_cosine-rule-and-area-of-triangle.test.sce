# Test: Cosine rule and area of triangle (A-3)
# DEG/RAD: DEG
# DPS: 3

load bin/sanyo-cz-0911pg/mathematics/A-03_cosine-rule-and-area-of-triangle.lst

/run
PS 3

# c = sqrt(a^2 + b^2 - 2ab cos alpha)
# GOTO 0 0, input a=sqrt(3), b=2, alpha=30 deg
GOTO 0 0
3 SQRT R/S
2 R/S
3 0 R/S
expect 1.000

# S = 1/2 ab sin alpha
# GOTO 3 0, input a=sqrt(3), b=2, alpha=30 deg
GOTO 3 0
3 SQRT R/S
2 R/S
3 0 R/S
expect 0.866
