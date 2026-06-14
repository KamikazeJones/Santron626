# HEADER
# Program Title: Covariance and correlation coefficient
# Program No.  : B-11
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# Sxy = [sum(xi yi) - (sum(xi) sum(yi) / n)] / sqrt(n - 1)
#
#          Sxy
# r = -----------
#      Sx * Sy
#
# EXAMPLES
# <Input>              <Output>
# (x1, y1) = (4, 6)    Sxy = 1.70
# (x2, y2) = (5, 9)    r = 0.26
# (x3, y3) = (1, 4)
# (x4, y4) = (7, 3)
# (x5, y5) = (2, 2)
#
# OPERATION
# 1. GOTO 0 0
# 2. Input of xi
# 3. *
# 4. Input of yi
# 5. R/S
# 6. Repeat steps 2 through 5
# 7. SKP (or GOTO 2 4)
# 8. RCL 3
# 9. R/S
# 10. RCL 1
# 11. R/S
#     Display of Sx
# 12. STO 6
# 13. RCL 4
# 14. R/S
# 15. RCL 2
# 16. R/S
#     Display of Sy
# 17. STO 7
# 18. GOTO 4 4
# 19. R/S
#     Display of Sxy
# 20. R/S
#     Display of r
#
# NOTES
# 1. Clear all memories before the operation.
#
# DATA MEMORY
# M0: sum(xi yi)
# M1: sum(xi)
# M2: sum(yi)
# M3: sum(xi^2)
# M4: sum(yi^2)
# M5: n
# M6: Sx
# M7: Sy
#
# Key name mapping used in this repository:
# F+ -> M+
# FX -> M*
# F- -> M-
# F÷ -> M/
# F√X -> X^2
# X↔Y -> X<>Y

/LOAD

# Source Code
M+ 2                         # 0 F+ 2
X^2                          # 2 F√X
M+ 4                         # 3 F+ 4
SQRT                         # 5 √X
X<>Y                         # 6 X↔Y
M+ 1                         # 7 F+ 1
X^2                          # 9 F√X
M+ 3                         # 10 F+ 3
SQRT                         # 12 √X
=                            # 13 =
M+ 0                         # 14 F+ 0
1                            # 16 1
M+ 5                         # 17 F+ 5
R/S                          # 19
GOTO 0 0                     # 20 GTO 0 0
R/S                          # 23
-                            # 24 -
(                            # 25 (5
R/S                          # 26
X^2                          # 27 F√X
/                            # 28 ÷
RCL 5                        # 29 RM
=                            # 31 =
/                            # 32 ÷
(                            # 33 (5
RCL 5                        # 34 RM
-                            # 36 -
1                            # 37 1
=                            # 38 =
SQRT                         # 39 √X
GOTO 2 3                     # 40 GTO 2 3
R/S                          # 43
RCL 0                        # 44 RM
-                            # 46 -
(                            # 47 (5
RCL 1                        # 48 RM
*                            # 50 X
RCL 2                        # 51 RM
/                            # 53 ÷
RCL 5                        # 54 RM
=                            # 56 =
/                            # 57 ÷
(                            # 58 (5
RCL 5                        # 59 RM
-                            # 61 -
1                            # 62 1
=                            # 63 =
R/S                          # 64
/                            # 65 ÷
RCL 6                        # 66 RM
/                            # 68 ÷
RCL 7                        # 69 RM
=                            # 71 =
