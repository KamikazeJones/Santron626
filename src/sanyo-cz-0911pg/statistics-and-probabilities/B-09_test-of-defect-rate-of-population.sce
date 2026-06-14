# HEADER
# Program Title: Test of defect rate of population
# Program No.  : B-9
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# p = r / n
#
#          p - P0
# U0 = ----------------
#      sqrt(P0 (1-P0) / n)
#
# where:
# p  : defect rate of sample
# P0 : defect rate of population
# n  : number of sample
# r  : number of defects
# U0 : statistics
#
# When p < 0.5 and r = np > 3, approximation method to the normal
# distribution is used.
#
# EXAMPLES
# <Input>              <Output>
# r = 50               P = 0.25
# n = 200              U0 = 1.63
# P0 = 0.20
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of r
# 4. R/S
# 5. Input of n
# 6. R/S
#    Display of P
# 7. Input of P0
# 8. R/S
#    Display of U0
# 9. Repeat steps 3 through 8.
#
# NOTES
# M0 stores n, M1 stores P, and M2 stores P - P0.
#
# DATA MEMORY
# M0: n
# M1: P
# M2: P - P0
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# √X -> SQRT

/LOAD

# Source Code
R/S                          # 0 Input of r or display of U0
/                            # 1 ÷
R/S                          # 2 Input of n
STO 0                        # 3 SM 0
=                            # 5 =  -> P = r / n
STO 1                        # 6 SM 1
-                            # 8 -
R/S                          # 9 Input of P0 or display of P
=                            # 10 = -> P - P0
STO 2                        # 11 SM 2
1                            # 13 1
-                            # 14 -
RCL 1                        # 15 RM 1
*                            # 17 X
RCL 1                        # 18 RM 1
/                            # 20 ÷
RCL 0                        # 21 RM 0
=                            # 23 = -> (1 - P) * P / n
SQRT                         # 24 √X
1/X                          # 25 1/X
*                            # 26 X
RCL 2                        # 27 RM 2
=                            # 29 = -> U0
GOTO 0 0                     # 30 GTO 0 0
