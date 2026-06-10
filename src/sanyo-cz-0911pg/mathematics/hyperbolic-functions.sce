# HEADER
# Program Title: Hyperbolic functions
# Program No.  : A-1
# DEG/RAD      : Arbitrary
# DPS          : 3
#
# FORMULA
# 1.  sinh x = 1/2(e^x - e^(-x))
# 2.  cosh x = 1/2(e^x + e^(-x))
# 3.  tanh x = (e^x - e^(-x)) / (e^x + e^(-x))
# 4. csech x = 1/sinh x
# 5.  sech x = 1/cosh x
# 6.  coth x = 1/tanh x
#
# EXAMPLES
# <input>    <output>
# x=2        sinh 2 = 3.627   csech 2 = 0.276
#            cosh 2 = 3.762    sech 2 = 0.266
#            tanh 2 = 0.964    coth 2 = 1.037
#
# OPERATION
#  1. GOTO 0 0
#  2. Input of Data x
#  3. R/S
#  4. Display of sinh x
#  5. GOTO 1 0
#  6. Input of Data x
#  7. R/S
#  8. Display of cosh x
#  9. GOTO 2 0
# 10. Input of Data x
# 11. R/S
# 12. Display of tanh x
#
# NOTES
# csech x, sech x and coth x can be obtained after the operation
# No. 4, No 8. and No. 12 respectivly, by pressing the key 1/x.
#
# DATA MEMORY
# M0: e^x

/LOAD

# Source Code
e^x - x<>y 1/x / 2 = R/S # display of sinh
SST                      # pad to address 10 (entry point for cosh) 
SST 
e^x + x<>y 1/x / 2 = R/S # display of cosh
SST                      # pad to address 20 (entry point for tanh)
SST
e^x STO 0 - x<>y 1/x /
( RCL 0 + x<>y 1/x )
= R/S                    # display of tanh x





