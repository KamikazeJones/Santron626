# HEADER
# Program Title: Interval estimation
# Program No.  : B-10
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# The case of 95% confidence coefficient.
#
# m = np,  sigma = sqrt(npq),  q = 1 - p
#
# Using binomial distribution:
# np - 2 sqrt(npq) <= r <= np + 2 sqrt(npq)
#
# where:
# p     : probability
# m     : mean value of binomial distribution
# sigma : standard deviation
#
# EXAMPLES
# <Input>              <Output>
# n = 100              upper limit = 74.54
# P = 0.65             lower limit = 55.46
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of n
# 4. R/S
# 5. Input of P
# 6. R/S
#    Display of upper limit
# 7. R/S
#    Display of lower limit
# 8. Repeat steps 3 through 7.
#
# DATA MEMORY
# M0: P or 2 sqrt(npq)
# M1: np
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# √X -> SQRT
# +↔- -> +/-

/LOAD

# Source Code
R/S                          # 0 Input of n or display of lower limit
*                            # 1 X
R/S                          # 2 Input of P
STO 0                        # 3 SM
=                            # 5 =
STO 1                        # 6 SM
1                            # 8 1
-                            # 9 -
RCL 0                        # 10 RM
*                            # 12 X
RCL 1                        # 13 RM
*                            # 15 X
4                            # 16 4
=                            # 17 =
SQRT                         # 18 √X
STO 0                        # 19 SM
+                            # 21 +
RCL 1                        # 22 RM
=                            # 24 =
R/S                          # 25 Display of upper limit
-                            # 26 -
(                            # 27 (5
RCL 1                        # 28 RM
*                            # 30 X
2                            # 31 2
)                            # 32 5)
=                            # 33 =
+/-                          # 34 +↔-
GOTO 0 0                     # 35 GTO 0 0
