# HEADER
# Program Title: Quadratic equation
# Program No.  : A-7
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# The roots x1, x2 of ax^2 + bx + c = 0 (a != 0)
# are given by x1, x2 = (-b +- sqrt(b^2 - 4ac)) / 2a
#
# EXAMPLES
# <input>   <output>
# a = 3     x1 = 0.33
# b = 2     x2 = -1.00
# c = -1
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of a
# 4. R/S
# 5. Input of b
# 6. R/S
# 7. Input of c
# 8. R/S
#    Display of x1
# 9. R/S
#    Display of x2
# 10. Repeat steps 3 through 9
#
# NOTES
# 1. If b^2 - 4ac < 0, "error" is displayed
#
# DATA MEMORY
# M0: working
# M1: b / 2a

# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# XY -> X<>Y
# F X -> X^2
# F SQRT -> SQRT
# +/− -> +/-

/LOAD

# Source Code
R/S                          # 0 Input of a or display of x2
STO 0                        # 1 SM 0
*                            # 3 x
2                            # 4 2
/                            # 5 ÷
R/S                          # 6 Input of b
X<>Y                         # 7 X↔Y
=                            # 8 =
STO 1                        # 9 SM 1
X^2                          # 11 F X
-                            # 12 -
(                            # 13 (
R/S                          # 14 Input of c
/                            # 15 ÷
RCL 0                        # 16 RM 0
)                            # 18 )
=                            # 19 =
SKP                          # 20 SKP
GOTO 2 8                     # 21 GTO 2 8
SQRT                         # 24 √X
GOTO 0 0                     # 25 GTO 0 0
SQRT                         # 28 √X
STO 0                        # 29 SM 0
-                            # 31 -
RCL 1                        # 32 RM 1
=                            # 34 =
R/S                          # 35 Display of x1
RCL 1                        # 36 RM 1
+                            # 38 +
RCL 0                        # 39 RM 0
=                            # 41 =
+/-                          # 42 +/-
GOTO 0 0                     # 43 GTO 0 0
