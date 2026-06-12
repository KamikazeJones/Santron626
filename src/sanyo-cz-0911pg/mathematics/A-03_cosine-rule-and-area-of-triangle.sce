# HEADER
# Program Title: Cosine rule and area of triangle
# Program No.  : A-3
# DEG/RAD      : DEG
# DPS          : 3
#
# FORMULA
# Given two sides and an included angle, the remaining side and the area of the
# triangle are computed by the following formulas:
#
# c = sqrt(a^2 + b^2 - 2ab cos alpha)
# S = 1/2 ab sin alpha
#
# DIAGRAM
# Triangle with base b, side a, side c, and included angle alpha between
# sides a and b. The program computes the remaining side c and area S.
#
# EXAMPLES
# <input>          <output>
# a=sqrt(3)        c = 1.000
# b=2              S = 0.866
# alpha=30 deg
#
# OPERATION
# Calculation of c:
#  1. GOTO 0 0
#  2. Input of a
#  3. R/S
#  4. Input of b
#  5. R/S
#  6. Input of alpha
#  7. R/S
#  8. Display of c
#
# Calculation of S:
#  1. GOTO 3 0
#  2. Input of a
#  3. R/S
#  4. Input of b
#  5. R/S
#  6. Input of alpha
#  7. R/S
#  8. Display of S
#
# DATA MEMORY
# M0: a
# M1: b

/LOAD

# Source Code
STO 0
X^2
+
R/S                         # input of b
STO 1
X^2
-
(
R/S                         # input of alpha
COS
*
RCL 0
*
RCL 1
*
2
=
SQRT
R/S                         # display of c

SST
SST
SST
SST
SST
SST
SST

*
R/S                         # input of b
*
R/S                         # input of alpha
SIN
/
2
=
R/S                         # display of S
