# Sanyo Library Extraction Workflow

This document describes the workflow for extracting programs from the scanned
Sanyo CZ-0911PG Program Library into source, listing, and regression-test files.

## Page Number Rule

The scanned/OCR file number is one greater than the printed page number from
the index.

Example:

```text
printed page 25 -> page-026_ocr.pdf
```

Use the index in `docs/sanyo-library.md` to find the printed page, then add one
to locate the scan file.

## Prepare A Page

From WSL:

```bash
cd /mnt/c/Users/quass/Documents/Programmieren/Santron626
bash tools/prepare-sanyo-page.sh /mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-011_ocr.pdf
```

The script writes files to:

```text
work/sanyo-pages/page-011/
```

Useful outputs:

- `page.png`: full rendered page
- `page.txt`: OCR/text extraction with layout
- `program-table.png`: cropped program table
- `main-text.png`: title and formula area
- `examples.png`: example area
- `operation-notes.png`: operation and notes area
- `draft.sce`: initial source template with raw OCR text

The OCR text is only a starting point. For program tables, always verify against
the rendered image because OCR often misreads calculator key symbols.

## Create The Source File

Create a commented source file under the category from the booklet index:

```text
src/sanyo-cz-0911pg/<category>/<program-name>.sce
```

Use this structure:

```text
# HEADER
# Program Title: ...
# Program No.  : ...
# DEG/RAD      : ...
# DPS          : ...
#
# FORMULA
# ...
#
# EXAMPLES
# ...
#
# OPERATION
# ...
#
# NOTES
# ...
#
# DATA MEMORY
# ...

/LOAD

# Source Code
...
```

If the page contains a diagram, describe it textually:

```text
# DIAGRAM
# Triangle with sides a, b, c and opposite angles A, B, C.
```

Do not guess from OCR alone. If a symbol is unclear, inspect `page.png` or
`program-table.png`.

## Translate Program Keys

Translate the Sanyo table into our textual key names:

```text
SM   -> STO
RM   -> RCL
GTO  -> GOTO
F sqrt with note x^2 -> X^2
sqrt -> SQRT
Fe^x with note Ln -> LN
R/S  -> R/S
```

Use `SST` as padding only when the original program table intentionally leaves
empty addresses before the next entry point.

Keep the source close to the original program table. If a program relies on
calculator behavior such as `=` evaluating an open parenthesis, preserve the
original key sequence and fix the emulator/model if necessary.

## Verify The Source

First check the generated listing:

```bash
node santron-cli.js --scenario-file src/sanyo-cz-0911pg/mathematics/inverse-hyperbolic-functions.sce --scenario "list"
```

Confirm that:

- entry points match the operation instructions, for example `GOTO 0 0`
- padding addresses are correct
- key names and keycodes match the program table

Then run the page examples:

```bash
node santron-cli.js \
  --scenario-file src/sanyo-cz-0911pg/mathematics/inverse-hyperbolic-functions.sce \
  --scenario ":RUN PS 4 GOTO 0 0 0 . 5 R/S; expect 0.4812" \
  --steps 200
```

For formulas, independently check values with JavaScript when useful:

```bash
node -e "const x=0.5; console.log(Math.log(Math.sqrt((1+x)/(1-x))))"
```

## Create The Strict Listing

After the source has been verified, create the strict numeric listing under:

```text
bin/sanyo-cz-0911pg/<category>/<program-name>.lst
```

The listing should contain all addresses from `00` through `71`. Unused cells
must be `099`.

Example format:

```text
00 055 STO
01 100 0
02 018 X^2
...
71 099
```

## Create The Regression Test

Create a scenario test under:

```text
tests/sanyo-cz-0911pg/<category>/<program-name>.test.sce
```

Use the examples from the page:

```text
# Test: Inverse hyperbolic functions (A-2)
# DPS: 4

load bin/sanyo-cz-0911pg/mathematics/inverse-hyperbolic-functions.lst

/run
PS 4

GOTO 0 0
0 . 5 R/S
expect 0.4812
```

Run all tests:

```bash
npm test
```

Run the GUI smoke test when the model changed:

```bash
npm run gui:smoke -- https://localhost:8765
```

## Update The Tracker

Update `docs/sanyo-library.md`:

- set `src` to `yes` after the `.sce` exists
- set `bin` to `yes` after the strict `.lst` exists
- set `test` to `yes` after the regression test exists

Keep category placement aligned with the booklet index.

## Commit Checklist

Before committing:

```bash
git status
npm test
```

If `santron-core.js` changed:

```bash
node --check santron-core.js
npm run gui:smoke -- https://localhost:8765
```

Do not commit `work/` outputs unless explicitly needed; they are preparation
artifacts.
