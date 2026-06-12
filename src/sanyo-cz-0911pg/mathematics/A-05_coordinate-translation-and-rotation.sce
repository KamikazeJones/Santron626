# HEADER
# Program Title: Coordinate translation and rotation
# Program No.  : A-5
# DEG/RAD      : DEG
# DPS          : 3
#
# FORMULA
# The origin is translated from O (0, 0) to a new point, O' (x0, y0), and the
# X and Y axes are then rotated through an angle theta to give new axes, X' and
# Y'. Suppose that a point P has coordinates (x, y) with respect to the old
# system of X and Y axes.
#
# The following formulas are given to find the coordinates (x', y') of P with
# respect to new system.
#
# x' = (x - x0) cos theta + (y - y0) sin theta
# y' = -(x - x0) sin theta + (y - y0) cos theta
#
# DIAGRAM
# The origin is translated to O' (x0, y0) and the X and Y axes are rotated to
# X' and Y'. Point P is shown in the new coordinate system.
#
# EXAMPLES
# <input>        <output>
# x0 = 2         x' = 2.828
# y0 = 1         y' = 1.7 x 10^-8 = 0
# theta = 45
# x = 4
# y = 3
#
# OPERATION
# 1. GOTO 0 0
# 2. Input of x
# 3. R/S
# 4. Input of y
# 5. R/S
# 6. Display of x'
# 7. R/S
# 8. Display of y'
# 9. Repeat steps 1 through 6
#
# NOTES
# The values of M0, M1 and M2 must be entered precedingly by users.
#
# DATA MEMORY
# M0: x0
# M1: y0
# M2: theta
# M3: cos theta
# M4: sin theta
# M5: x
# M6: y

# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# +↔- -> +/-

/LOAD

# Source Code
STO 5                        # 0 SM 5
-                            # 2 -
RCL 0                        # 3 RM 0
*                            # 5 x
RCL 2                        # 6 RM 2
COS                          # 8 COS
STO 3                        # 9 SM 3
+                            # 11 +
(                            # 12 (
(                            # 13 (
R/S                          # 14 R/S Input of y
STO 6                        # 15 SM 6
-                            # 17 -
RCL 1                        # 18 RM 1
)                            # 20 )
*                            # 21 x
RCL 2                        # 22 RM 2
SIN                          # 24 SIN
STO 4                        # 25 SM 4
=                            # 27 =
R/S                          # 28 Display of x'
RCL 5                        # 29 RM 5
-                            # 31 -
RCL 0                        # 32 RM 0
*                            # 34 x
RCL 4                        # 35 RM 4
+/-                          # 37 +/-
+                            # 38 +
(                            # 39 (
(                            # 40 (
RCL 6                        # 41 RM 6
-                            # 43 -
RCL 1                        # 44 RM 1
)                            # 46 )
*                            # 47 x
RCL 3                        # 48 RM 3
=                            # 50 =
R/S                          # 51 Display of y'
