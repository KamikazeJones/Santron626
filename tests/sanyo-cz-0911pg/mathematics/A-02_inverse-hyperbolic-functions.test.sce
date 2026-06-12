# Test: Inverse hyperbolic functions (A-2)
# DPS: 4

load bin/sanyo-cz-0911pg/mathematics/A-02_inverse-hyperbolic-functions.lst

/run
PS 4

# sinh^-1 x: GOTO 0 0, input x, R/S
GOTO 0 0
0 . 5 R/S
expect 0.4812

# cosh^-1 x: GOTO 2 0, input x, R/S
GOTO 2 0
5 R/S
expect 2.2924

# tanh^-1 x: GOTO 4 0, input x, R/S
GOTO 4 0
0 . 5 R/S
expect 0.5493
