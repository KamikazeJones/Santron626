# HEADER
# Program Title: Chi-square evaluation
# Program No.  : B-14
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# chi^2 = sum(((xi - mi)^2) / mi)
#
# where:
# xi: observed frequency
# mi: expected frequency
#
# EXAMPLES
# <Input>              <Output>
# x1 = 115             chi^2 = 4.50
# x2 = 85
# m1 = 100
# m2 = 100
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of xi
# 4. R/S
# 5. Input of mi
# 6. R/S
# 7. Repeat steps 3 through 6.
#    After entry of all data
# 8. SKP
# 9. SKP
# 10. R/S
#     Display of chi^2
#
# NOTES
# Clear M2 before the operation.
#
# DATA MEMORY
# M0: xi
# M1: mi
# M2: sum(((xi - mi)^2) / mi)
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F√X -> X^2
# F+ -> M+

/LOAD

# Source Code
R/S                          # 0 Input of xi
STO 0                        # 1 SM
R/S                          # 3 Input of mi
STO 1                        # 4 SM
RCL 0                        # 6 RM
-                            # 8 -
RCL 1                        # 9 RM
=                            # 11 =
X^2                          # 12 F√X
/                            # 13 ÷
RCL 1                        # 14 RM
=                            # 16 =
M+ 2                         # 17 F+ 2
GOTO 0 0                     # 19 GTO 0 0
R/S                          # 22
RCL 2                        # 23 RM 2
