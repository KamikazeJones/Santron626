# Sanyo CZ-0911PG Program Library

This file tracks the Sanyo CZ-0911PG Program Library and our extraction
progress. It is based on pages 2-6 of the scanned library booklet.

## Page Format

Each program sheet in the booklet uses the same structure:

- `PROGRAM TITLE`: General title of the program.
- `DEG/RAD`: Required position of the DEG/RAD slide switch. If the sheet says
  `Arbitrary`, the switch position is irrelevant.
- `DPS`: Required decimal point setting. Before running the program, press
  `DPS`/`PS` and then the specified digit `N` (`0` through `9`).
- `PROGRAM NO.`: Classification and number inside the library.
- `FORMULA`: Formulae and expressions used by the program.
- `EXAMPLES`: Concrete input data and expected output data.
- `OPERATION`: User operation after entering the program in LOAD mode and
  switching back to RUN mode.
- `NOTES`: Initial data settings, special operation notes, and other references.
- `DATA MEMORY`: Meaning of memory registers used by the program.
- `PROGRAM`: Key operations for entering the program in LOAD mode. The `NOTE`
  column beside the program table may describe individual instructions or
  named entry points.

## Extraction Status

Status columns:

- `src`: commented `.sce` source exists under `src/sanyo-cz-0911pg/...`
- `bin`: strict numeric `.lst` exists under `bin/sanyo-cz-0911pg/...`
- `test`: regression scenario exists under `tests/sanyo-cz-0911pg/...`

The scanned/OCR file number is one greater than the printed page number from
the index. For example, printed page `25` is stored as `page-026_ocr.pdf`.

### A. Mathematics

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| A-1 | Hyperbolic functions | 9 | yes | yes | yes |
| A-2 | Inverse hyperbolic functions | 10 | yes | yes | yes |
| A-3 | Cosine rule and area of triangle | 11 | yes | yes | yes |
| A-4 | Angle between two straight lines | 12 | yes | yes | yes |
| A-5 | Coordinate translation and rotation | 13 | yes | yes | yes |
| A-6 | Simultaneous equations in 2 unknowns | 14 | yes | yes | yes |
| A-7 | Quadratic equation | 15 | yes | yes | yes |
| A-8 | Complex arithmetic (+, -, x) | 16 | yes | yes | yes |
| A-9 | Determinant and inverse of a 2x2 matrix | 17 | yes | yes | yes |
| A-10 | Simpson's rule for numerical integration | 18 | yes | yes | yes |
| A-11 | Numerical solution to differential equations | 19 | yes | yes | yes |
| A-12 | Base conversion (number in base b to number in base 10) | 20 | yes | yes | yes |
| A-13 | Base conversion (number in base 10 to number in base b) | 21 | yes | yes | yes |

### B. Statistics and Probabilities

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| B-1 | Arithmetic mean (Non distribution) | 25 | yes | yes | yes |
| B-2 | Arithmetic mean (Frequency distribution) | 26 | yes | yes | yes |
| B-3 | Geometric mean | 27 | yes | yes | yes |
| B-4 | Mean, variance, standard deviation | 28 | yes | yes | yes |
| B-5 | Moving average (3-terms) | 29 | yes | yes | yes |
| B-6 | Logarithmic curve fit | 30 | yes | yes | yes |
| B-7 | Exponential curve fit | 31 | yes | yes | yes |
| B-8 | Estimation of population mean (95% confidence interval) | 32 | yes | yes | yes |
| B-9 | Test of defect rate of population | 33 | yes | yes | yes |
| B-10 | Interval estimation | 34 | yes | yes | yes |
| B-11 | Covariance and correlation coefficient | 35 | yes | yes | yes |
| B-12 | Binominal coefficient (nCr) | 36 | yes | yes | yes |
| B-13 | Permutation (nPr) | 37 | yes | yes | yes |
| B-14 | Chi-square evaluation | 38 | yes | yes | yes |
| B-15 | Normal distribution | 39 | yes | yes | yes |
| B-16 | Inverse normal integral | 40 | yes | yes | yes |
| B-17 | Random number generator | 41 | yes | yes | yes |

### C. Surveying

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| C-1 | Open traverse | 45 | no | no | no |
| C-2 | Closed traverse | 46 | no | no | no |
| C-3 | Area of polygon | 47 | no | no | no |
| C-4 | Simple curve setting by middle ordinates | 48 | no | no | no |
| C-5 | Calculation of the excluded length and area by corner cutting | 49 | no | no | no |
| C-6 | Calculation of the corner cutting of the chord and circle (Inside the circle) | 50 | no | no | no |
| C-7 | Stadia survey | 51 | no | no | no |
| C-8 | Reduction to center (Eccentric observed point) | 52 | no | no | no |

### D. Electrical Engineering

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| D-1 | C, R Circuit | 55 | no | no | no |
| D-2 | Biot-savart's law | 56 | no | no | no |
| D-3 | Intensity of magnetic flux at the center axis for circular coil | 57 | no | no | no |
| D-4 | LRC series resonant circuit | 58 | no | no | no |
| D-5 | Circuit design for constant K band pass filter | 59 | no | no | no |
| D-6 | Delta <-> Y Conversion | 60 | no | no | no |
| D-7 | Electrostatic deflection for cathode ray tube | 61 | no | no | no |
| D-8 | Intensity of illumination (Point source of light) | 62 | no | no | no |

### E. Architecture

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| E-1 | Maximum flexure of beam (Both ends fixed) | 65 | no | no | no |
| E-2 | Flexure of fixed beam (Rectangular section) | 66 | no | no | no |
| E-3 | Flexure of Grain Garter | 67 | no | no | no |
| E-4 | Planning of slab (Cantilever beam) | 68 | no | no | no |

### F. Civil Engineering

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| F-1 | Rectangular orifice | 71 | no | no | no |
| F-2 | Siphon (Flow velocity, quantity of flow) | 72 | no | no | no |
| F-3 | Velocity formula | 73 | no | no | no |
| F-4 | Mean quality of flow/flow velocity (Manning's formula) | 74 | no | no | no |
| F-5 | Calculating stability of upright earth slope | 75 | no | no | no |
| F-6 | Coulomb's coefficient of active earth pressure | 76 | no | no | no |
| F-7 | Rankine's active earth pressure | 77 | no | no | no |
| F-8 | Stress of reinforced concrete beam | 78 | no | no | no |

### G. Mechanical Engineering

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| G-1 | Intermeshing rare of standard gear | 81 | no | no | no |
| G-2 | Diameter of base circle of helical gear | 82 | no | no | no |
| G-3 | Length of belt | 83 | no | no | no |
| G-4 | Thermal loss | 84 | no | no | no |
| G-5 | Heat-conduction between two plates | 85 | no | no | no |
| G-6 | Involute function | 86 | no | no | no |

### H. General Business

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| H-1 | Loan calculation (1) | 89 | no | no | no |
| H-2 | Loan calculation (2) | 90 | no | no | no |
| H-3 | Loan calculation (3) | 91 | no | no | no |
| H-4 | Compound interest | 92 | no | no | no |
| H-5 | Periodic savings (Payment, present value, number of periods) | 93 | no | no | no |
| H-6 | Discount cash flow (Net present, internal rate of return) | 94 | no | no | no |
| H-7 | Calculation of salary | 95 | no | no | no |
| H-8 | Determination of economical lot size (1) | 96 | no | no | no |
| H-9 | Determination of economical lot size (2) | 97 | no | no | no |
| H-10 | Calendar (Days between two days) | 98 | no | no | no |
| H-11 | Calendar (Day of the week) | 99 | no | no | no |
| H-12 | Calculation of biorhythm | 100 | no | no | no |
| H-13 | Game calculation (A fox who pursue a rabbit) | 101 | no | no | no |
| H-14 | Conjectural game | 102 | no | no | no |

### I. Unit Conversion

| No. | Program | Page | src | bin | test |
| --- | --- | ---: | :-: | :-: | :-: |
| I-1 | Angle conversion (Degree, radian) | 105 | no | no | no |
| I-2 | Unit conversion (deg C, deg F, deg K) (On temperature) | 106 | no | no | no |
| I-3 | Unit conversion (pounds, ounces, grams) (On weight) | 107 | no | no | no |
| I-4 | Unit conversion (gallons (us) <-> Liters, cm^3 <-> In^3) (In volume) | 108 | no | no | no |

## Notes

- The category letters in this tracker follow the booklet index.
- Some titles appear to contain original spelling or wording oddities; they are
  intentionally kept close to the source until the corresponding program page is
  extracted.
