# Test: Covariance and correlation coefficient (B-11)
# DPS: 2

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-11_covariance-and-correlation-coefficient.lst

/run
PS 2

/CLR
/RUN
GOTO 0 0
4 * 6 R/S
5 * 9 R/S
1 * 4 R/S
7 * 3 R/S
2 * 2 R/S
SKP
RCL 3
R/S
RCL 1
R/S
expect 2.39
STO 6
RCL 4
R/S
RCL 2
R/S
expect 2.77
STO 7
GOTO 4 4
R/S
expect 1.70
R/S
expect 0.26
