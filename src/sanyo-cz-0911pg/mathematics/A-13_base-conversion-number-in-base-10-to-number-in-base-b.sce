# HEADER
# Program Title: Base conversion (Number in base 10 to number in base b)
# Program No.  : A-13
# DEG/RAD      : Arbitrary
# DPS          : 0
#
# FORMULA
# Conversion from arbitrary decimal number N10
# to b-ary number Nb (2<=b<=100). Obtaining from lower integer
# digit to upper by repeat calculation, then fraction number is
# obtained from upper fraction digit to lower by repeat calculation.
#
# EXAMPLES
# 1. 455.625 (Decimal) to hexadecimal notation
#    (input)   455.625
#    (output)  1C7.A
# 2. 0.7525 degree (Decimal) to sexagecimal degree
#    (input)   0.7525
#    (output)  45' 09"
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of positive decimal number
# 4. R/S
# 5. Input of base number (b)
# 6. R/S
#    Display of lowest integer digit number.
# 7. Obtain next integer digit number in order by repetition of step no.6.
# 8. SKP
# 9. R/S
#    Display of 1st fraction number
# 10. Obtain next fraction number by repetition of step no.9.
#
# NOTES
# 1. Example of base number input
#    hexadecimal -> 16
#    sexagecimal -> 60
# 2. If the display becomes succession of zero by repeat
#    operation of step number 6,
#    operator can advance next step
#    by pressing SKP key.
# 3. Result of each digit number
#    will be displayed
#    by using 2 digit
#    display region:
#    example,
#    hexadecimal
#    display  output
#    0-9      0-9
#    10       A
#    11       B
#    12       C
#    13       D
#    14       E
#    15       F
#
# DATA MEMORY
# M0: base b
# M1: working
# M2: working
# M3: Decimal number
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F- -> M-
# F X -> M*
# X↔Y -> X<>Y

/LOAD

# Source Code
R/S                          # 0 Input of decimal number
STO 3                        # 1 SM 3
+                            # 3 +
EXP                          # 4 EXP
9                            # 5 9
-                            # 6 -
EXP                          # 7 EXP
9                            # 8 9
=                            # 9 =
STO 1                        # 10 SM 1
M- 3                         # 12 F- 3
R/S                          # 14 Input of base b
STO 0                        # 15 SM 0
RCL 1                        # 17 RM 1
STO 2                        # 19 SM 2
/                            # 21 ÷
RCL 0                        # 22 RM 0
+                            # 24 +
EXP                          # 25 EXP
9                            # 26 9
-                            # 27 -
EXP                          # 28 EXP
9                            # 29 9
=                            # 30 =
STO 1                        # 31 SM 1
*                            # 33 X
RCL 0                        # 34 RM 0
-                            # 36 -
RCL 2                        # 37 RM 2
X<>Y                         # 39 X↔Y
=                            # 40 =
R/S                          # 41 Display of integer part
GOTO 1 7                     # 42 GTO 1 7
R/S                          # 45
RCL 0                        # 46 RM 0
M* 3                         # 48 F X 3
RCL 3                        # 50 RM 3
+                            # 52 +
EXP                          # 53 EXP
9                            # 54 9
-                            # 55 -
EXP                          # 56 EXP
9                            # 57 9
=                            # 58 =
M- 3                         # 59 F- 3
R/S                          # 61 Display of fraction part
GOTO 4 6                     # 62 GTO 4 6
