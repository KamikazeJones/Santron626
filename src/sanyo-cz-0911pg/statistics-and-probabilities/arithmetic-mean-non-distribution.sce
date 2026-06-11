# HEADER
# Program Title: Arithmetic mean (Non distribution)
# Program No.  : B-1
# DEG/RAD      : Arbitrary
# DPS          : 3
#
# FORMULA
# This program computes the arithmetic mean x_bar for a set of given
# data x1, x2, ..., xn.
#
# x_bar = (1 / n) * sum(xi), i = 1..n
#
# where
# xi: given data
# n : number of given data
#
# EXAMPLES
# <input>     <output>
# x1 = 9.140  x_bar = 9.165
# x2 = 9.145
# x3 = 9.165
# x4 = 9.185
# x5 = 9.190
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of xi
# 4. R/S
# 5. Repeat steps 3 and 4.
#    After entry of all data
# 6. SKP
# 7. R/S
# 8. Display of x_bar
# 9. Repeat steps 1 through 7.
#
# NOTES
# Every time data is input and R/S key is pressed, the number of
# data is displayed.
#
# DATA MEMORY
# M0: sum(xi)
# M1: n

/LOAD

# Source Code
C/CE                         # clear display and expression state
STO 0                        # initialize sum in M0 to 0
STO 1                        # initialize count in M1 to 0
R/S                          # wait for xi, or display n/x_bar
M+ 0                         # add xi to the running sum
1 M+ 1                       # increment n
RCL 1                        # display n after each input
GOTO 0 5                     # return to the input stop
R/S                          # skipped when all data has been entered
RCL 0 / RCL 1 =              # calculate sum(xi) / n
GOTO 0 5                     # return to the display/input stop
