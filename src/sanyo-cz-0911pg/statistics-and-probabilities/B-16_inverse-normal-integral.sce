# HEADER
# Program Title: Inverse normal integral
# Program No.  : B-16
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# For 0 < Q <= 0.5 this program approximates X from
#
#                 1      / X        -t^2/2
# Q = --------- * |    e        dt
#     sqrt(2 pi)  / 0
#
# with
#
# X = t - (C0 + C1 t + C2 t^2) / (1 + d1 t + d2 t^2 + d3 t^3)
#
# and
#
# t = sqrt(ln(1 / Q^2))
#
# where |epsilon(Q)| < 4.5 x 10^-4
#
# EXAMPLES
# <Input>              <Output>
# Q = 0.0013499        X = 3.00
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of Q
# 4. R/S
#    Display of X
# 5. Repeat steps 3 through 4
#
# NOTES
# Before operation, store C0 through C2 and d1 through d3 into
# M0 through M5 respectively.
#
# C0 = 2.515517
# C1 = 0.802853
# C2 = 0.010328
# d1 = 1.432788
# d2 = 0.189269
# d3 = 0.001308
#
# DATA MEMORY
# M0: C0 = 2.515517
# M1: C1 = 0.802853
# M2: C2 = 0.010328
# M3: d1 = 1.432788
# M4: d2 = 0.189269
# M5: d3 = 0.001308
# M6: t
# M7: t^2
# M8: t^3
#
# Key name mapping used in this repository:
# F√X -> X^2
# F e^x with note Ln -> LN
# F X -> M*
# SM -> STO
# RM -> RCL

/LOAD

# Source Code
R/S                          # 0 Input of Q or display of X
X^2                          # 1 F√X
1/X                          # 2 1/X
LN                           # 3 F e^x note Ln
SQRT                         # 4 √X
STO 6                        # 5 SM 6
STO 8                        # 7 SM 8
X^2                          # 9 F√X
STO 7                        # 10 SM 7
M* 8                         # 12 F X 8
RCL 6                        # 14 RM 6
-                            # 16 -
(                            # 17 (5
RCL 0                        # 18 RM 0
+                            # 20 +
(                            # 21 (5
RCL 1                        # 22 RM 1
*                            # 24 X
RCL 6                        # 25 RM 6
)                            # 27 5)
+                            # 28 +
(                            # 29 (5
RCL 2                        # 30 RM 2
*                            # 32 X
RCL 7                        # 33 RM 7
)                            # 35 5)
/                            # 36 ÷
(                            # 37 (5
1                            # 38 1
+                            # 39 +
(                            # 40 (5
RCL 3                        # 41 RM 3
*                            # 43 X
RCL 6                        # 44 RM 6
)                            # 46 5)
+                            # 47 +
(                            # 48 (5
RCL 4                        # 49 RM 4
*                            # 51 X
RCL 7                        # 52 RM 7
)                            # 54 5)
+                            # 55 +
(                            # 56 (5
RCL 5                        # 57 RM 5
*                            # 59 X
RCL 8                        # 60 RM 8
)                            # 62 5)
)                            # 63 5)
)                            # 64 5)
=                            # 65 Calculation of X
GOTO 0 0                     # 66 GTO 0 0
