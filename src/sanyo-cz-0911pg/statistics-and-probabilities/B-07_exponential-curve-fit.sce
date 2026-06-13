# HEADER
# Program Title: Exponential curve fit
# Program No.  : B-7
# DEG/RAD      : Arbitrary
# DPS          : 5
#
# FORMULA
# This program calculates coefficients a and b for an
# exponential curve
#
# y = a e^(b x)
#
# b = (sum(xi ln(yi)) - (1 / n) * sum(xi) * sum(ln(yi)))
#     / (sum(xi^2) - (1 / n) * (sum(xi))^2)
#
# a = exp((sum(ln(yi)) / n) - b * (sum(xi) / n))
#
# where (xi, yi) is a set of data (i = 1, 2, ..., n)
# and ln(y) = ln(a) + b x
#
# EXAMPLES
# <Input>            <Output>
# (x1, y1) = (1, 1)  a = 0.50000
# (x2, y2) = (2, 2)  b = 0.69315
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of yi
# 4. R/S
# 5. Input of xi
# 6. R/S
# 7. Repeat steps 3 through 6.
#    After completion of all data entry
# 8. SKP
# 9. SKP
# 10. R/S
#     Display of a
#
# NOTES
# 1. Clear M0 through M5 before the operation.
# 2. b and a will be stored into M6 and M7 respectively.
#
# DATA MEMORY
# M0: sum(ln(yi))
# M1: sum(xi)
# M2: sum(xi^2)
# M3: working
# M4: sum(xi ln(yi))
# M5: n
# M6: b
# M7: a
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F+ -> M+
# F X -> M*
# F e^x with note ln -> LN
# F √X with note X^2 -> X^2
# e^x -> E^X
# +↔- -> +/-

/LOAD

# Source Code
R/S                          # 0 Input of y
LN                           # 1 F e^x note ln
M+ 0                         # 2 F+ 0
STO 3                        # 4 SM 3
R/S                          # 6 Input of x
M+ 1                         # 7 F+ 1
M* 3                         # 9 F X 3
X^2                          # 11 F √X note X^2
M+ 2                         # 12 F+ 2
RCL 3                        # 14 RM 3
M+ 4                         # 16 F+ 4
1                            # 18 1
M+ 5                         # 19 F+ 5
GOTO 0 0                     # 21 GTO 0 0
R/S                          # 24
RCL 1                        # 25 RM 1
*                            # 27 X
RCL 0                        # 28 RM 0
/                            # 30 ÷
RCL 5                        # 31 RM 5
-                            # 33 -
RCL 4                        # 34 RM 4
/                            # 36 ÷
(                            # 37 (5
RCL 1                        # 38 RM 1
X^2                          # 40 F √X note X^2
/                            # 41 ÷
RCL 5                        # 42 RM 5
-                            # 44 -
RCL 2                        # 45 RM 2
)                            # 47 5)
=                            # 48 =
STO 6                        # 49 SM 6
*                            # 51 X
RCL 1                        # 52 RM 1
-                            # 54 -
RCL 0                        # 55 RM 0
/                            # 57 ÷
RCL 5                        # 58 RM 5
+/-                          # 60 +↔-
=                            # 61 =
E^X                          # 62 e^x
STO 7                        # 63 SM 7
