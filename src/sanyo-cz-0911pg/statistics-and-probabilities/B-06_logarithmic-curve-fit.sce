# HEADER
# Program Title: Logarithmic curve fit
# Program No.  : B-6
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# This program fits a logarithmic curve
#
# y = a + b ln(x)
#
# for the set of data {(xi, yi), i = 1, 2, ..., n}
#
# a = (1 / n) * (sum(yi) - b * sum(ln(xi)))
#
# b = (sum(yi ln(xi)) - (1 / n) * sum(ln(xi)) * sum(yi))
#     / (sum((ln(xi))^2) - (1 / n) * (sum(ln(xi)))^2)
#
# EXAMPLES
# <Input>     <Output>
# xi   yi     a = -47.02
# 3    1.5    b =  41.39
# 4    9.3
# 6   23.4
# 10  45.8
# 12  60.1
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of xi
# 4. R/S
# 5. Input of yi
# 6. R/S
# 7. Repeat steps 3 through 6.
# 8. After completion of all data entry
#    SKP
# 9. SKP
# 10. R/S
#     Display of a
# 11. RM 7
#     Display of b
#
# NOTES
# 1. Clear M1 through M6 before the operation.
# 2. The printed operation sequence really needs the final
#    yi to be confirmed with R/S before the two SKP presses.
#
# DATA MEMORY
# M1: ln(xi)
# M2: sum(ln(xi))
# M3: sum((ln(xi))^2)
# M4: n
# M5: sum(yi)
# M6: sum(yi ln(xi))
# M7: b
# M8: a
# M9: 1 / n
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F+ -> M+
# F e^x with note ln -> LN
# F √X with note X^2 -> X^2
# 1/X -> 1/X
# +↔- -> +/-

/LOAD

# Source Code
R/S                          # 0 Input of x
LN                           # 1 F e^x note ln
STO 1                        # 2 SM 1
M+ 2                         # 4 F+ 2
X^2                          # 6 F √X note X^2
M+ 3                         # 7 F+ 3
1                            # 9 1
M+ 4                         # 10 F+ 4
R/S                          # 12 Input of y
M+ 5                         # 13 F+ 5
*                            # 15 X
RCL 1                        # 16 RM 1
=                            # 18 =
M+ 6                         # 19 F+ 6
GOTO 0 0                     # 21 GTO 0 0
R/S                          # 24
RCL 4                        # 25 RM 4
1/X                          # 27 1/X
STO 9                        # 28 SM 9
*                            # 30 X
RCL 2                        # 31 RM 2
*                            # 33 X
RCL 5                        # 34 RM 5
-                            # 36 -
RCL 6                        # 37 RM 6
/                            # 39 ÷
(                            # 40 (5
RCL 9                        # 41 RM 9
*                            # 43 X
RCL 2                        # 44 RM 2
X^2                          # 46 F √X note X^2
-                            # 47 -
RCL 3                        # 48 RM 3
)                            # 50 5)
=                            # 51 =
STO 7                        # 52 SM 7
*                            # 54 X
RCL 2                        # 55 RM 2
-                            # 57 -
RCL 5                        # 58 RM 5
/                            # 60 ÷
RCL 4                        # 61 RM 4
=                            # 63 =
+/-                          # 64 +↔-
STO 8                        # 65 SM 8
