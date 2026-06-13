# HEADER
# Program Title: Estimation of population mean (95% confidence interval)
# Program No.  : B-8
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# x_bar - (2 S / sqrt(n)) <= m <= x_bar + (2 S / sqrt(n))
#
# lower limit                   upper limit
#
# EXAMPLES
# Estimate the mean height of schoolboys of N town
#
# <Input>              <Output>
# number of sample     157.20 <= m <= 159.80
# n = 100
# standard deviation
# S = 6.5 cm
# mean x_bar = 158.5 cm
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of x_bar
# 4. R/S
# 5. Input of S
# 6. R/S
# 7. Input of n
# 8. R/S
#    Display of lower limit
# 9. R/S
#    Display of upper limit
# 10. Repeat steps 3 through 9
#
# NOTES
# M0 stores the upper limit, so the second R/S simply recalls it.
#
# DATA MEMORY
# M0: upper limit
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F+ -> M+

/LOAD

# Source Code
R/S                          # 0 Input of x_bar or display of upper limit
STO 0                        # 1 SM 0
-                            # 3 -
(                            # 4 (5
R/S                          # 5 Input of S
/                            # 6 ÷
R/S                          # 7 Input of n
SQRT                         # 8 √X
*                            # 9 X
2                            # 10 2
)                            # 11 5)
M+ 0                         # 12 F+ 0
=                            # 14 =
R/S                          # 15 Display of lower limit
RCL 0                        # 16 RM 0
GOTO 0 0                     # 18 GTO 0 0
