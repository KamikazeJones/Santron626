# HEADER
# Program Title: Moving average (3-terms)
# Program No.  : B-5
# DEG/RAD      : Arbitrary
# DPS          : 3
#
# FORMULA
# This program calculates the moving average of
# three terms for data x1, x2, ..., xn.
#
# x_bar1 = (x1 + x2 + x3) / 3
# x_bar2 = (x2 + x3 + x4) / 3
# ...
# x_bari = (xi + x(i+1) + x(i+2)) / 3
#
# EXAMPLES
# <Input>      <Output>
# x1 = 3.4     x_bar1 = 3.400
# x2 = 3.3     x_bar2 = 3.500
# x3 = 3.5     x_bar3 = 3.600
# x4 = 3.7     x_bar4 = 3.633
# x5 = 3.6     x_bar5 = 3.700
# x6 = 3.6     x_bar6 = 3.867
# x7 = 3.9
# x8 = 4.1
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of x1
# 4. R/S
# 5. Input of x2
# 6. R/S
# 7. Input of x3
# 8. R/S
#    Display of x_bar1
# 9. Repeat steps 7 and 8.
#    Display of x_bari
#
# NOTES
# The printed program shifts the window by relying on the
# calculator's pending-operation behavior:
# STO 1 stores x(i+1), STO 2 stores x(i+2), while the sums are
# only resolved when the next operator key is pressed.
#
# DATA MEMORY
# M1: xi
# M2: x(i+1)
# M3: x(i+2)
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL

/LOAD

# Source Code
R/S                          # 0 Input of x1
STO 1                        # 1 SM 1
R/S                          # 3 Input of x2
STO 2                        # 4 SM 2
R/S                          # 6 Input of x3 or display of x_bar1
STO 3                        # 7 SM 3
RCL 1                        # 9 RM 1
+                            # 11 +
RCL 2                        # 12 RM 2
STO 1                        # 14 SM 1
+                            # 16 +
RCL 3                        # 17 RM 3
STO 2                        # 19 SM 2
/                            # 21 ÷
3                            # 22 3
=                            # 23 =
GOTO 0 6                     # 24 GTO 0 6
