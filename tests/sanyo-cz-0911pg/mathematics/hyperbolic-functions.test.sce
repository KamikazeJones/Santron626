# Test: Hyperbolic functions (A-1)
# DPS: 3

load bin/sanyo-cz-0911pg/mathematics/hyperbolic-functions.lst

/run
PS 3

# sinh x: GOTO 0 0, input x, R/S
GOTO 0 0
2 R/S
expect 3.627

# csech x: 1/x after sinh
1/x
expect 0.276

# cosh x: GOTO 1 0, input x, R/S
GOTO 1 0
2 R/S
expect 3.762

# sech x: 1/x after cosh
1/x
expect 0.266

# tanh x: GOTO 2 0, input x, R/S
GOTO 2 0
2 R/S
expect 0.964

# coth x: 1/x after tanh
1/x
expect 1.037
