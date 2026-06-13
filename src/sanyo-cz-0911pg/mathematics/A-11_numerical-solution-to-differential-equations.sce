# HEADER
# Program Title: Numerical solution to differential equations
# Program No.  : A-11
# DEG/RAD      : Arbitrary
# DPS          : 7
#
# FORMULA
# Let y' = f(x, y) be a first order differential
# equations with initial values x0, y0.
# The solution is a numerical solution, which calculates yi for
# xi = x0 + ih, where h is an increment specified by the user and
# i = 1, 2, ...
#
# The program uses a modified Euler method:
# y_hat_(i+1) = yi + h f(xi, yi)
# y_(i+1) = yi + (h / 2) [f(xi, yi) + f(x_(i+1), y_hat_(i+1))]
#
# The definition of function f(x, y) is performed by storing into the
# program memory the information of key operation required for
# determining f(x, y).
#
# EXAMPLES
# y' = y, y(0) = 1 (Answer y = e^x)
# <Input>                    <Output>
# h = 0.01                   y(0.01) = 1.0100500
# x0 = 0                     (true value 1.0100502)
# y0 = 1                     y(0.02) = 1.0202010
#                            (true value 1.0202013)
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of h
# 4. R/S
# 5. Input of x0
# 6. R/S
# 7. Input of y0
# 8. R/S
# 9. Display of y(x0 + h)
# 10. R/S
#     Display of y(x0 + 2h)
#     ...
#
# NOTES
# 1. The definition of function
#    f(x, y) is performed by
#    storing into the program
#    memory the information
#    of key operation required
#    for determining
#    f(x, y).
#
# DATA MEMORY
# M0: h
# M1: x_(i+1)
# M2: y_(i+1)
# M3: working
# M4: xi
# M5: yi
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# C -> C/CE
# X -> *
# F+ -> M+
# F- -> M-

/LOAD

# Source Code
R/S                          # 0 Input of h
STO 0                        # 1 SM 0
R/S                          # 3 Input of x0
STO 1                        # 4 SM 1
STO 4                        # 6 SM 4
R/S                          # 8 Input of y0
STO 2                        # 9 SM 2
STO 5                        # 11 SM 5
C/CE                         # 13 C
STO 3                        # 14 SM 3
RCL 2                        # 16 RM 2
SST                          # 18 f(x, y), printed blank after RM 2 for the example y' = y
SST                          # 19
SST                          # 20
SST                          # 21
SST                          # 22
SST                          # 23
SST                          # 24
SST                          # 25
SST                          # 26
*                            # 27 X
RCL 0                        # 28 RM 0
=                            # 30 =
M+ 3                         # 31 F+ 3
RCL 4                        # 33 RM 4
-                            # 35 -
RCL 1                        # 36 RM 1
=                            # 38 =
SKP                          # 39 SKP
GOTO 6 1                     # 40 GTO 6 1
M- 4                         # 43 F- 4
RCL 3                        # 45 RM 3
/                            # 47 ÷
2                            # 48 2
+                            # 49 +
RCL 5                        # 50 RM 5
=                            # 52 =
R/S                          # 53 Display of yi+1
STO 2                        # 54 SM 2
STO 5                        # 56 SM 5
GOTO 1 3                     # 58 GTO 1 3
RCL 0                        # 61 RM 0
M+ 1                         # 63 F+ 1
RCL 3                        # 65 RM 3
M+ 2                         # 67 F+ 2
GOTO 1 6                     # 69 GTO 1 6
