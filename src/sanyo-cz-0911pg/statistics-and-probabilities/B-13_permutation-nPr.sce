# HEADER
# Program Title: Permutation (nPr)
# Program No.  : B-13
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# nPr = n! / (n-r)! = n (n-1) ... (n-r+1)
#
# EXAMPLES
# <Input>              <Output>
# r = 2                nPr = 20.00
# n = 5
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of r
# 4. R/S
# 5. Input of n
# 6. R/S
#    Display of nPr
#
# DATA MEMORY
# M0: r
# M1: n - i
# M3: nPr
# M9: n - r + 1
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F X -> M*

/LOAD

# Source Code
1                            # 0 1
STO 3                        # 1 SM
R/S                          # 3 Input of r
STO 0                        # 4 SM
R/S                          # 6 Input of n
STO 1                        # 7 SM
-                            # 9 -
RCL 0                        # 10 RM
+                            # 12 +
1                            # 13 1
=                            # 14 =
STO 9                        # 15 SM
RCL 1                        # 17 RM
M* 3                         # 19 F X 3
-                            # 21 -
1                            # 22 1
=                            # 23 =
STO 1                        # 24 SM
-                            # 26 -
RCL 9                        # 27 RM
=                            # 29 =
SKP                          # 30 SKIP
GOTO 1 7                     # 31 GTO 1 7
RCL 3                        # 34 RM 3
