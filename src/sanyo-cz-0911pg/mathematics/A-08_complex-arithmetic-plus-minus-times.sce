# HEADER
# Program Title: Complex arithmetic (+, -, X)
# Program No.  : A-8
# DEG/RAD      : Arbitrary
# DPS          : 0
#
# FORMULA
# Let z1 = a1 + b1 i and z2 = a2 + b2 i be two
# complex numbers. The arithmetic operations +, -, X are defined
# as follows.
#
# 1. z1 + z2 = (a1 + a2) + (b1 + b2) i
# 2. z1 - z2 = (a1 - a2) + (b1 - b2) i
# 3. z1 z2 = (a1 a2 - b1 b2) + (a1 b2 + b1 a2) i
#
# EXAMPLES
# <input>            <output>
# a1 = 1             z1 + z2 = 4 + 6i
# b1 = 2             z1 - z2 = -2 - 2i
# a2 = 3             z1 z2 = -5 + 10i
# b2 = 4
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
#    Display of Real part (a1 + a2)
#    of addition
# 3. R/S
#    Display of Imaginary Part
#    (b1 + b2) of addition
# 4. R/S
#    Display of Real part (a1 - a2)
#    of subtraction
# 5. R/S
#    Display of Imaginary Part
#    (b1 - b2) of subtraction
# 6. R/S
#    Display of Real part (a1 a2 -
#    b1 b2) of multiplication
# 7. R/S
#    Display of Imaginary Part
#    (a1 b2 + b1 a2) of multiplication
#
# NOTES
# 1. The values of M0, M1, M2
#    and M3 must be entered
#    precedingly by users.
#
# DATA MEMORY
# M0: a1
# M1: b1
# M2: a2
# M3: b2
#
# Key name mapping used in this repository:
# RM -> RCL
# X -> *

/LOAD

# Source Code
RCL 0                        # 0 RM 0
+                            # 2 +
RCL 2                        # 3 RM 2
=                            # 5 =
R/S                          # 6 R/S Display of (a1 + a2)
RCL 1                        # 7 RM 1
+                            # 9 +
RCL 3                        # 10 RM 3
=                            # 12 =
R/S                          # 13 R/S Display of (b1 + b2)
RCL 0                        # 14 RM 0
-                            # 16 -
RCL 2                        # 17 RM 2
=                            # 19 =
R/S                          # 20 R/S Display of (a1 - a2)
RCL 1                        # 21 RM 1
-                            # 23 -
RCL 3                        # 24 RM 3
=                            # 26 =
R/S                          # 27 R/S Display of (b1 - b2)
RCL 0                        # 28 RM 0
*                            # 30 X
RCL 2                        # 31 RM 2
-                            # 33 -
(                            # 34 (
RCL 1                        # 35 RM 1
*                            # 37 X
RCL 3                        # 38 RM 3
=                            # 40 =
R/S                          # 41 R/S Display of (a1 a2 - b1 b2)
RCL 0                        # 42 RM 0
*                            # 44 X
RCL 3                        # 45 RM 3
+                            # 47 +
(                            # 48 (
RCL 1                        # 49 RM 1
*                            # 51 X
RCL 2                        # 52 RM 2
=                            # 54 =
R/S                          # 55 R/S Display of (a1 b2 + b1 a2)
