# HEADER
# Program Title: Angle between two straight lines
# Program No.  : A-4
# DEG/RAD      : DEG
# DPS          : 3
#
# FORMULA
# This program calculates the angle between two
# straight lines, y1 = m1x + b1 and y2 = m2x + b2
#
# theta = tan^-1 ( (m1 - m2) / (1 + m1m2) )
#
# Where theta is the angle measured clockwise
# from m1 line to m2 line.
#
# DIAGRAM
# Two straight lines crossing at angle theta.
#
# EXAMPLES
# <input>        <output>
# y1 = 1/2 x + 2      theta = 71.565
# y2 = -x + 5
#
# OPERATION
# 1. GOTO 0 0
# 2. Input of m1
# 3. R/S
# 4. Input of m2
# 5. R/S
# Display of theta
# 6. Repeat steps 1 through 5
#
# NOTES
#
#
# DATA MEMORY
# M0: m1
# M1: m2

# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F TAN -> ATAN

/LOAD

# Source Code
STO 0                        # 0 SM 0
-                            # 2 -
R/S                          # 3 R/S Input of m2
STO 1                        # 4 SM 1
/                            # 6 /
(                            # 7 (
1                            # 8 1
+                            # 9 +
(                            # 10 (
RCL 0                        # 11 RM 0
*                            # 13 x
RCL 1                        # 14 RM 1
=                            # 16 =
ATAN                         # 17 F TAN
R/S                          # 18 R/S Display of theta
