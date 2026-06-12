# HEADER
# Program Title: Inverse hyperbolic functions
# Program No.  : A-2
# DEG/RAD      : Arbitrary
# DPS          : 4
#
# FORMULA
# 1. sinh^-1 x = Ln(x + sqrt(x^2 + 1))       (-inf < x < +inf)
# 2. cosh^-1 x = Ln(x + sqrt(x^2 - 1))       (1 <= x)
# 3. tanh^-1 x = Ln sqrt((1 + x) / (1 - x))  (-1 < x < +1)
#
# EXAMPLES
# <input>  <output>
# x=0.5    sinh^-1 0.5 = 0.4812
# x=5      cosh^-1 5   = 2.2924
# x=0.5    tanh^-1 0.5 = 0.5493
#
# OPERATION
#  1. GOTO 0 0
#  2. Input of Data x
#  3. R/S
#  4. Display of sinh^-1 x
#  5. GOTO 2 0
#  6. Input of Data x
#  7. R/S
#  8. Display of cosh^-1 x
#  9. GOTO 4 0
# 10. Input of Data x
# 11. R/S
# 12. Display of tanh^-1 x
#
# DATA MEMORY
# M0: x

/LOAD

# Source Code
STO 0                        # save x
X^2 + 1 = SQRT + RCL 0 = LN R/S

SST
SST
SST
SST
SST
SST
SST

STO 0                        # save x
X^2 - 1 = SQRT + RCL 0 = LN R/S

SST
SST
SST
SST
SST
SST
SST

STO 0                        # save x
+ 1 / ( 1 - RCL 0 = SQRT LN R/S
