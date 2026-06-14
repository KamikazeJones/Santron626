# HEADER
# Program Title: Random number generator
# Program No.  : B-17
# DEG/RAD      : Arbitrary
# DPS          : 4
#
# FORMULA
# This program calculates uniformly distributed pseudo random numbers
# u(i) in the range 0 <= u(i) <= 1.
#
# u(i) = Fractional part of (PI + u(i-1))^5
#
# The user has to specify the starting value u(0) such that
# 0 <= u(0) <= 1.
#
# EXAMPLES
# <input>       <output>
# u(0)=0.1200   u(1) = 0.1039
#               u(2) = 0.0677
#               u(3) = 0.4677
#
# EMULATOR DEVIATION
# u(3) = 0.4676
#
# OPERATION
# 1. GOTO 0 0
# 2. R/S
# 3. Input of Data u(0)
# 4. R/S, Display of u(i)
# 5. Repeat step 4

/LOAD

# Source Code
R/S                         # wait for the initial seed or show the next value
+ PI y^x 5 =                # calculate (u(i-1) + PI)^5
STO 0                       # keep the full intermediate value in M0
+ EXP 9 - EXP 9 -           # truncate integer digits: add 1E9, subtract 1E9
RCL 0 x<>y =                # subtract the truncated part from the saved value
GOTO 0 0                    # loop back to the R/S stop at address 00
