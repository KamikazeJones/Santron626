# HEADER
# Program Title: Simultaneous equations in 2 unknowns
# Program No.  : A-6
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# Let a1x1 + b1x2 = c1
# and a2x1 + b2x2 = c2
# be a system of two equations in two unknowns.
#
# The following formulas are given to find the solution.
#
# x1 = (b1c2 - b2c1) / (a2b1 - a1b2)
# x2 = (c1 - a1x1) / b1
#
# EXAMPLES
# <input>    <output>
# a1 = 1     x1 = 1.00
# b1 = -1    x2 = 2.00
# c1 = -1
# a2 = 1
# b2 = 1
# c2 = 3
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
#    Display of x1
# 3. R/S
#    Display of x2
#
# NOTES
# 1. The values of M0, M1, M2,
#    M3, M4 and M5 must be
#    entered precedingly by users.
#
# DATA MEMORY
# M0: a1
# M1: b1
# M2: c1
# M3: a2
# M4: b2
# M5: c2
# M6: x1

# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL

/LOAD

# Source Code
RCL 1                        # 0 RM 1
*                            # 2 x
RCL 5                        # 3 RM 5
-                            # 5 -
(                            # 6 (
RCL 4                        # 7 RM 4
*                            # 9 x
RCL 2                        # 10 RM 2
)                            # 12 )
/                            # 13 /
(                            # 14 (
RCL 3                        # 15 RM 3
*                            # 17 x
RCL 1                        # 18 RM 1
-                            # 20 -
(                            # 21 (
RCL 0                        # 22 RM 0
*                            # 24 x
RCL 4                        # 25 RM 4
=                            # 27 =
STO 6                        # 28 SM 6
R/S                          # 30 Display of x1
RCL 2                        # 31 RM 2
-                            # 33 -
(                            # 34 (
RCL 0                        # 35 RM 0
*                            # 37 x
RCL 6                        # 38 RM 6
)                            # 40 )
/                            # 41 /
RCL 1                        # 42 RM 1
=                            # 44 =
R/S                          # 45 Display of x2
