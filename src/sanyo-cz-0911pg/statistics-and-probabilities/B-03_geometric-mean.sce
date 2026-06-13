# HEADER
# Program Title: Geometric mean
# Program No.  : B-3
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# The following formula is given to obtain geometric
# mean of n quantites (x_g_bar)
#
# x_g_bar = n-th root of (x1 x2 x3 ..... xn)
#         = (product from i=1 to n of xi)^(1/n)
#
# EXAMPLES
# <Input>      <Output>
# x1 = 3.4     x_g_bar = 6.61
# x2 = 5.1
# x3 = 10.2
# x4 = 10.8
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of xi
# 4. R/S
#    ...
#    xn
#    R/S
# 5. SKP
# 6. R/S
#    Display of x_g_bar
#
# NOTES
# 1. Number of input data be
#    obtained by pressing RM 1
#
# DATA MEMORY
# M0: product xi
# M1: n
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F X -> M*
# F+ -> M+
# YX -> Y^X
# 1/X -> 1/X

/LOAD

# Source Code
1                            # 0 1
STO 0                        # 1 SM 0
R/S                          # 3 Input of x1
M* 0                         # 4 F X 0
1                            # 6 1
M+ 1                         # 7 F+ 1
GOTO 0 3                     # 9 GTO 0 3
R/S                          # 12 SKP or R/S
RCL 0                        # 13 RM 0
Y^X                          # 15 YX
RCL 1                        # 16 RM 1
1/X                          # 18 1/X
=                            # 19 =
