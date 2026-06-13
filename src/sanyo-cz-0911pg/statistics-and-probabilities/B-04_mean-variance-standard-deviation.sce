# HEADER
# Program Title: Mean, Variance, Standard Deviation
# Program No.  : B-4
# DEG/RAD      : Arbitrary
# DPS          : 6
#
# FORMULA
# This program calculates the mean x_bar, the
# variance r and the standard deviation sigma
#
# x_bar = sum(xi) / n
# r = sum((xi - x_bar)^2) / n
# sigma = sqrt(r)
#
# EXAMPLES
# <Input>      <Output>
# x1 = 9.140   x_bar = 9.165000
# x2 = 9.145   r = 0.000413
# x3 = 9.165   sigma = 0.020322
# x4 = 9.185
# x5 = 9.190
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of xi
# 4. R/S
# 5. Repeat steps 3 and 4.
# 6. After entry of all data
#    SKP
# 7. R/S
#    Display of sigma
# 8. RM 4
#    Display of x_bar
# 9. RM 5
#    Display of r
#
# NOTES
# 1. Clear M1, M2, and M3
#    before the operation.
# 2. The printed example output for r and sigma
#    does not match the formula on the page or
#    the transcribed program table exactly.
#    With the printed inputs, the program
#    yields r = 0.000410 and sigma = 0.020248
#    before DPS rounding.
#
# DATA MEMORY
# M1: sum x
# M2: sum x^2
# M3: n
# M4: x_bar
# M5: r
# M6: sigma
#
# Key name mapping used in this repository:
# RM -> RCL
# F+ -> M+
# F √X -> X^2
# +↔- -> +/-

/LOAD

# Source Code
R/S                          # 0 Input of xi
M+ 1                         # 1 F+ 1
X^2                          # 3 F √X
M+ 2                         # 4 F+ 2
1                            # 6 1
M+ 3                         # 7 F+ 3
GOTO 0 0                     # 9 GTO 0 0
R/S                          # 12
RCL 1                        # 13 RM 1
/                            # 15 ÷
RCL 3                        # 16 RM 3
=                            # 18 =
STO 4                        # 19 SM 4
X^2                          # 21 F √X
*                            # 22 X
RCL 3                        # 23 RM 3
-                            # 25 -
RCL 2                        # 26 RM 2
/                            # 28 ÷
RCL 3                        # 29 RM 3
=                            # 31 =
+/-                          # 32 +↔-
STO 5                        # 33 SM 5
SQRT                         # 35 √X
STO 6                        # 36 SM 6
