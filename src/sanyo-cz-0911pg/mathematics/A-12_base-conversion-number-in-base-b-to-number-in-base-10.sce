# HEADER
# Program Title: Base conversion (Number in base b to number in base 10)
# Program No.  : A-12
# DEG/RAD      : Arbitrary
# DPS          : 5
#
# FORMULA
# This program consists of two programs.
# 1. To change the integer part of a number in base b to a number
#    in base 10.
#    I10 = in in-1 ... i2 i1 = in b^(n-1) + i(n-1) b^(n-2) + ..... + i2 b + i1
#        = b (..... (b (b (in) + i(n-1)) + i(n-2)) + .....) + i2) + i1
#
# 2. To change the fraction part of a number in base b to a number
#    in base 10.
#    F10 = f1 f2 ..... fm = f1 b^-1 + f2 b^-2 + ..... + fm b^-m
#
# By combining these two programs, any number in base b can be
# converted to a number in base 10.
#
# EXAMPLES
# <Input>      <Output>
# 10101 (2)    21.00000 (10)
# 0.10101 (2)  0.65625 (10)
#
# OPERATION
# Integer part                 Fraction part
# 1. GOTO 0 0                 1. GOTO 2 0
# 2. Input of in              2. Input of fm
# 3. R/S                      3. R/S
# 4. Input of in-1            4. Input of fm-1
# 5. Repeat steps 3 and 4.    5. Repeat steps 3 and 4.
#    After entry of all data     After entry of all data
# 6. =                        6. R/S
#    Display of I10              Display of F10
#
# NOTES
# 1. Before the operation, store b
#    into memory M0.
#
# DATA MEMORY
# M0: b
#
# Key name mapping used in this repository:
# RM -> RCL
# X -> *
# GTO -> GOTO

/LOAD

# Source Code
*                            # 0 X
RCL 0                        # 1 RM 0
+                            # 3 +
R/S                          # 4 Input of in-r--i1 or display of I10
=                            # 5 =
GOTO 0 0                     # 6 GTO 0 0
SST                          # 9 printed blank
SST                          # 10
SST                          # 11
SST                          # 12
SST                          # 13
SST                          # 14
SST                          # 15
SST                          # 16
SST                          # 17
SST                          # 18
SST                          # 19
/                            # 20 ÷
RCL 0                        # 21 RM 0
+                            # 23 +
R/S                          # 24 Input of fm-r--f1 or display of F10
=                            # 25 =
GOTO 2 0                     # 26 GTO 2 0
