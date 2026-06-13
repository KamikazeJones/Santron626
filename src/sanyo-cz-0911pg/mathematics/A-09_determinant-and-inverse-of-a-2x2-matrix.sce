# HEADER
# Program Title: Determinant and inverse of a 2X2 matrix
# Program No.  : A-9
# DEG/RAD      : Arbitrary
# DPS          : 1
#
# FORMULA
# Let r = [r11 r12; r21 r22] be a 2X2 matrix.
#
# The determinant of r denoted by Det r is evaluated by the
# following formula:
# Det r = r22 r11 - r12 r21
#
# The multiplicative inverse r^-1 of r is evaluated by the following
# formula:
# r^-1 = [r22 / Det r   -r12 / Det r;
#         -r21 / Det r   r11 / Det r]
#
# EXAMPLES
# <input>              <output>
# r = [1 3; 2 4]       Det r = -2.0
#                      r^-1 = [-2.0   1.5;
#                               1.0  -0.5]
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
#    Display of Det r
# 3. R/S
#    Display of r22 / Det r
# 4. R/S
#    Display of -r12 / Det r
# 5. R/S
#    Display of -r21 / Det r
# 6. R/S
#    Display of r11 / Det r
#
# NOTES
# 1. The values of M0, M1, M2 and M3 must be entered
#    precedingly by users.
#
# DATA MEMORY
# M0: r11
# M1: r12
# M2: r21
# M3: r22
# M4: Det r
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# X -> *
# +↔- -> +/-

/LOAD

# Source Code
RCL 3                        # 0 RM 3
*                            # 2 X
RCL 0                        # 3 RM 0
-                            # 5 -
(                            # 6 (
RCL 1                        # 7 RM 1
*                            # 9 X
RCL 2                        # 10 RM 2
=                            # 12 =
STO 4                        # 13 SM 4
R/S                          # 15 R/S Display of Det r
RCL 3                        # 16 RM 3
/                            # 18 ÷
RCL 4                        # 19 RM 4
=                            # 21 =
R/S                          # 22 R/S Display of r22/Det r
RCL 1                        # 23 RM 1
/                            # 25 ÷
RCL 4                        # 26 RM 4
=                            # 28 =
+/-                          # 29 +↔-
R/S                          # 30 R/S Display of -r12/Det r
RCL 2                        # 31 RM 2
/                            # 33 ÷
RCL 4                        # 34 RM 4
=                            # 36 =
+/-                          # 37 +↔-
R/S                          # 38 R/S Display of -r21/Det r
RCL 0                        # 39 RM 0
/                            # 41 ÷
RCL 4                        # 42 RM 4
=                            # 44 =
R/S                          # 45 R/S Display of r11/Det r
