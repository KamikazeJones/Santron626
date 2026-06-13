# HEADER
# Program Title: Arithmetic mean (Frequency distribution)
# Program No.  : B-2
# DEG/RAD      : Arbitrary
# DPS          : 2
#
# FORMULA
# This program calculates arithmetic mean x_bar for a
# set of data which has frequency distribution.
#
# Class value: x1, x2, x3, ........, xi, ........
# frequency : f1, f2, f3, ........, fi, ........
#
# x_bar = (1 / N) * sum(fi xi) (arithmetic mean)
#
# where N = sum(fi)
#       xi = x1 + (i - 1) h (i = 1, 2, ........., n)
#       x1 : initial class value
#       h  : class interval
#       fi : frequency
#
# EXAMPLES
# <Input>      <Output>
# h = 0.5      x_bar = 3.50
# x1 = 3       f1 = 3
#              f2 = 6
#              f3 = 8
#              f4 = 5
#              f5 = 2
#              f6 = 1
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of h
# 4. R/S
# 5. Input of x1
# 6. R/S
#    Display of xi
# 7. Input of fi or SKP
# 8. Repeat steps 6 and 7 to fn
# 9. R/S
#    Display of x_bar
# 10. Repeat steps 3 through 9.
#
# NOTES
# 1. SKP at 7 step operates to
#    obtain x_bar after an input of fi
# 2. The printed program table shows '-' at step 14.
#    That does not agree with the printed formula or
#    the printed example data. This repository uses '='
#    at step 14 as a documented booklet misprint correction.
# 3. The printed example becomes consistent only if
#    x1 is read as 2.5 instead of 3, and if the last
#    frequency is followed by R/S, then SKP, then R/S.
# 4. With x1 entered as 3 and the corrected '=' at step 14,
#    the program yields x_bar = 4.00 for the printed
#    frequencies, which matches the direct weighted-mean
#    calculation from the printed formula.
#
# DATA MEMORY
# M0: h
# M1: sum f
# M2: sum f x
# M3: x1 + n h
# M4: last input of f
#
# Key name mapping used in this repository:
# SM -> STO
# RM -> RCL
# F+ -> M+
# X -> *

/LOAD

# Source Code
R/S                          # 0 Input of h / Display of x_bar
STO 0                        # 1 SM 0
R/S                          # 3 Input of x1
STO 3                        # 4 SM 3
R/S                          # 6 Display of xi / Input of fi or SKP
STO 4                        # 7 SM 4
M+ 1                         # 9 F+ 1
*                            # 11 X
RCL 3                        # 12 RM 3
=                            # 14 Printed sheet shows '-', treated here as a misprint
M+ 2                         # 15 F+ 2
RCL 0                        # 17 RM 0
M+ 3                         # 19 F+ 3
RCL 3                        # 21 RM 3
GOTO 0 6                     # 23 GTO 0 6
R/S                          # 26
RCL 2                        # 27 RM 2
/                            # 29 ÷
RCL 1                        # 30 RM 1
=                            # 32 =
GOTO 0 0                     # 33 GTO 0 0
