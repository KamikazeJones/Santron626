# HEADER
# Program Title: Simpson's rule for numerical integration
# Program No.  : A-10
# DEG/RAD      : RAD
# DPS          : 4
#
# FORMULA
# Let x0, x1, ..., xn be equally spaced points such
# that xi = x0 + ih for i = 0, 1, 2, ......, n at which corresponding
# values f(x0), f(x1), ..., f(xn) of a function f(x) are known.
# Simpson's rule is:
# integral from x0 to xn of f(x) dx
#   ~= (h / 3) [f(x0) + 4f(x1) + 2f(x2) + ......
#               + 4f(xn-3) + 2f(xn-2) + 4f(xn-1) + f(xn)]
#
# DIAGRAM
# Graph of y = f(x) sampled at equally spaced points
# x0, x1, x2, ..., xn on the X axis.
#
# EXAMPLES
# <input>                              <output>
# h = pi / 8, f(x) = sin^2 x          S = integral from 0 to pi of sin^2 x dx
# i    : 0   1      2    3      4  5      6    7      8
# x_i  : 0  pi/8   pi/4 3pi/8  pi/2 5pi/8 3pi/4 7pi/8 pi
# f(xi): 0  0.1464 0.5  0.8536 1  0.8536 0.5  0.1464 0
#                                      = 1.5708
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input h
# 4. R/S
# 5. Input of f(x0)
# 6. R/S
# 7. Input of f(xn)
# 8. R/S
# 9. Input of f(xi)
# 10. R/S
# 11. Input of f(xi+1)
# 12. R/S
# 13. Repeat steps 9 through 12
#
# NOTES
# 1. Press RM 1, after the
#    completion of input of all
#    f(xi), then S is
#    displayed.
#
# DATA MEMORY
# M0: h / 3
# M1: working
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# X -> *
# F+ -> M+

/LOAD

# Source Code
R/S                          # 0 Input of h
/                            # 1 ÷
3                            # 2 3
=                            # 3 =
STO 0                        # 4 SM 0
*                            # 6 X
R/S                          # 7 Input of f(x0)
=                            # 8 =
STO 1                        # 9 SM 1
R/S                          # 11 Input of f(xn)
*                            # 12 X
RCL 0                        # 13 RM 0
=                            # 15 =
M+ 1                         # 16 F+ 1
R/S                          # 18 Input of f(xi)
*                            # 19 X
RCL 0                        # 20 RM 0
*                            # 22 X
4                            # 23 4
=                            # 24 =
M+ 1                         # 25 F+ 1
R/S                          # 27 Input of f(xi+1)
*                            # 28 X
RCL 0                        # 29 RM 0
*                            # 31 X
2                            # 32 2
=                            # 33 =
M+ 1                         # 34 F+ 1
GOTO 1 8                     # 36 GTO 1 8
