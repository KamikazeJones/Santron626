# HEADER
# Program Title: Normal distribution
# Program No.  : B-15
# DEG/RAD      : Arbitrary
# DPS          : 4
#
# FORMULA
# The density function for a standard normal
# distribution is
#
#            1         -x^2/2
# f(x) = --------- * e
#        sqrt(2 pi)
#
# The upper tail area is
#
#             1            / infinity   -t^2/2
# Q(x) = --------- *       |         e        dt
#        sqrt(2 pi)      / x
#
# where 0 <= x <= 3
#
# DIAGRAM
# Bell curve of f(x) with the upper tail Q(x) shaded from x to infinity.
#
# EXAMPLES
# <Input>              <Output>
# x = 2.0              Q(2) = 0.0228
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of x
# 4. R/S
#    Display of Q(x)
#
# NOTES
# Clear M3 before the operation.
#
# DATA MEMORY
# M0: xi
# M1: e^(-xi^2/2)
# M2: Delta
# M3: sum(e^(-xi^2/2))
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F+ -> M+
# FsqrtX -> X^2
# +<->- -> +/-
# X<->Y -> X<>Y
# GTO -> GOTO
# e^x -> E^X

/LOAD

# Source Code
.                            # 0 .
1                            # 1
STO 2                        # 2 SM 2
R/S                          # 4 Input of x
STO 0                        # 5 SM 0
X^2                          # 7 FsqrtX
/                            # 8 /
2                            # 9
+/-                          # 10 +<->-
=                            # 11
E^X                          # 12 e^x
STO 1                        # 13 SM 1
RCL 2                        # 15 RM 2
M+ 0                         # 17 F+ 0
RCL 0                        # 19 RM 0
X^2                          # 21 FsqrtX
/                            # 22 /
2                            # 23
+/-                          # 24 +<->-
=                            # 25
E^X                          # 26 e^x
M+ 3                         # 27 F+ 3
-                            # 29
RCL 1                        # 30 RM 1
X<>Y                         # 32 X<->Y
STO 1                        # 33 SM 1
/                            # 35 /
2                            # 36
=                            # 37
M+ 3                         # 38 F+ 3
3                            # 40
-                            # 41
RCL 0                        # 42 RM 0
=                            # 44
SKP                          # 45
GOTO 1 5                     # 46 GTO 1 5
PI                           # 49 pi
*                            # 50 X
2                            # 51
=                            # 52
SQRT                         # 53 sqrtX
/                            # 54 /
RCL 3                        # 55 RM 3
/                            # 57 /
RCL 2                        # 58 RM 2
=                            # 60
1/X                          # 61 1/X
+                            # 62
9                            # 63
7                            # 64
6                            # 65
1                            # 66
EXP                          # 67 EXP
7                            # 68
+/-                          # 69 +<->-
=                            # 70
SST                          # 71 blank
