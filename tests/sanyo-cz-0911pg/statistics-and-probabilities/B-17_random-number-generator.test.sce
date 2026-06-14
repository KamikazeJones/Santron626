# Test: Random number generator (B-17)
# DPS: 4

load bin/sanyo-cz-0911pg/statistics-and-probabilities/B-17_random-number-generator.lst

/run
PS 4

# The program starts at 0 0 and needs an initial R/S before the seed input.
GOTO 0 0
R/S

# u(0) = 0.1200
0 . 1 2 0 0 R/S
expect 0.1039

# u(1) -> u(2)
R/S
expect 0.0677

# u(2) -> u(3)
R/S
expect 0.4676
