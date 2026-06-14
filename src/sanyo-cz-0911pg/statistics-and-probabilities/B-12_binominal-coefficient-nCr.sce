# HEADER
# Program Title: Binominal coefficient (nCr)
# Program No.  : B-12
# DEG/RAD      : Arbitrary
# DPS          : 0
#
# FORMULA
# nCr = n! / (r! (n-r)!)
#
# EXAMPLES
# <Input>              <Output>
# r = 2                5C2 = 10
# n = 5
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of r
# 4. R/S
# 5. Input of n
# 6. R/S
#    Display of nCr
#
# DATA MEMORY
# M0: r
# M1: n!, r!
# M3: nPr, r!
# M4: nPr, r!
# M5: nPr, nCr
# M6: flag
# M9: n - r + 1
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F X -> M*
# F- -> M-
# F÷ -> M/

/LOAD

# Source Code
1                            # 0 1
STO 3                        # 1 SM
STO 6                        # 3 SM
R/S                          # 5 Input of r
STO 0                        # 6 SM
R/S                          # 8 Input of n
STO 1                        # 9 SM
-                            # 11 -
RCL 0                        # 12 RM
+                            # 14 +
1                            # 15 1
=                            # 16 =
STO 9                        # 17 SM
RCL 1                        # 19 RM
M* 3                         # 21 F X 3
-                            # 23 -
1                            # 24 1
=                            # 25 =
STO 1                        # 26 SM
-                            # 28 -
RCL 9                        # 29 RM
=                            # 31 =
SKP                          # 32 SKIP
GOTO 1 9                     # 33 GTO 1 9
RCL 4                        # 36 RM
STO 5                        # 38 SM
RCL 3                        # 40 RM
STO 4                        # 42 SM
2                            # 44 2
STO 9                        # 45 SM
RCL 0                        # 47 RM
STO 1                        # 49 SM
1                            # 51 1
STO 3                        # 52 SM
M- 6                         # 54 F- 6
RCL 6                        # 56 RM
SKP                          # 58 SKIP
GOTO 1 9                     # 59 GTO 1 9
RCL 4                        # 62 RM
M/ 5                         # 64 F÷ 5
RCL 5                        # 66 RM
