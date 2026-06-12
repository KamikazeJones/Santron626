# Sanyo Program Extraction Workflow

This document describes the current workflow for extracting Sanyo CZ-0911PG
library programs from OCR PDFs into Santron source, listing, tests, and
documentation.

## Purpose

The OCR text is useful for titles, formulas, examples, and notes, but it is not
reliable enough for the program key table. Symbols such as `SM`, `RM`, `R/S`,
`GTO`, `F+`, parentheses, and shifted functions are often misread or split into
columns. Treat extraction as a prepared, half-manual process:

1. Prepare the page with `tools/prepare-sanyo-page.sh`.
2. Read OCR text and page crops side by side.
3. Transcribe the program table from the rendered image.
4. Write a commented `.sce` source.
5. Build or verify the strict `.lst`.
6. Add regression examples from the original sheet.
7. Update `docs/sanyo-library.md`.

Do not trust OCR alone for the program table. Read the rendered page and the
cropped images side by side before writing the source.

## Required Tools

Run the preparation step in WSL/Linux. The script expects:

- `poppler-utils`: `pdftotext`, `pdftoppm`, `pdfinfo`
- `imagemagick`: `magick` or `convert` for crops
- Node.js for Santron CLI and tests

The script supports Windows paths when `wslpath` is available, but the most
predictable form is a WSL path such as:

```bash
/mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-011_ocr.pdf
```

If the scan is hard to read, rerun the preparation step at higher resolution:

```bash
bash tools/prepare-sanyo-page.sh -r 400 /mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-011_ocr.pdf
```

## Page Number Rule

The printed page number in the booklet index is one less than the scanned file
number:

```text
scanned/OCR file number = printed page number + 1
```

Example:

```text
printed page 10 -> page-011_ocr.pdf
printed page 25 -> page-026_ocr.pdf
```

Use `docs/sanyo-library.md` as the tracker for category, program number, title,
printed page, and extraction status.

## Prepare A Page

From the repository root:

```bash
bash tools/prepare-sanyo-page.sh /mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-011_ocr.pdf
```

The default output goes to:

```text
work/sanyo-pages/page-011/
```

Important generated files:

- `page.txt`: layout-preserving text from `pdftotext -layout`
- `page.png`: full page rendered at 300 dpi
- `program-table.png`: right-side program table crop
- `main-text.png`: title and formula crop
- `examples.png`: examples crop
- `operation-notes.png`: operation and notes crop
- `header.png`: header crop
- `draft.sce`: starter source with the OCR text embedded as comments
- `README.md`: generated local next steps

The most useful working files are usually:

- `page.txt`: layout-preserving OCR text
- `page.png`: the full rendered page
- `program-table.png`: the program table crop
- `main-text.png`: title and formula area
- `examples.png`: examples area
- `operation-notes.png`: operation and notes area

Treat `page.txt` as a starting point only. OCR often misreads calculator key
symbols, so verify the table against `program-table.png` and the full page
image.

For hard-to-read scans, increase resolution:

```bash
bash tools/prepare-sanyo-page.sh -r 400 /mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-011_ocr.pdf
```

## Extract The Sheet Content

Use the OCR text for broad structure, then verify against the image crops:

- `PROGRAM TITLE`
- `PROGRAM NO.`
- `DEG/RAD`
- `DPS`
- `FORMULA`
- `EXAMPLES`
- `OPERATION`
- `NOTES`
- `DATA MEMORY`
- `PROGRAM`

Keep the booklet structure unchanged. Transcribe the text blocks as written on
the page, including the original order of sections and the original phrasing of
headings, notes, examples, and operation text.

When a formula is graphical, convert it into text form for the repository, but
do not change its placement or the surrounding section structure. Keep the
formula block in the same section and preserve the book page's line order as
closely as practical.

If a title or phrase looks odd in the OCR, check `page.png` before correcting it
silently.

When the booklet contains a diagram, keep the repository text-first and
describe the shape or geometry in comments rather than adding image assets.

## Handle Diagrams

Some mathematics pages include triangles, lines, coordinate systems, or vector
diagrams. Keep the repository text-first:

- Describe the drawing in the `.sce` comments under a `# DIAGRAM` section.
- Use the drawing to identify variables and formulas correctly.
- Keep generated crops under `work/sanyo-pages/...` as working material.
- Do not add diagram images to the repository unless they become essential
  documentation assets.

Example:

```text
# DIAGRAM
# Triangle with sides a, b, c and opposite angles A, B, C.
# The program computes side/angle combinations using the cosine rule.
```

## Write The `.sce` Source

Create the final source under:

```text
src/sanyo-cz-0911pg/<category>/<slug>.sce
```

Use the category from the booklet index, not a guess from the program number.
For example, `B` is `statistics-and-probabilities`, while `H` is
`general-business`.

Recommended source shape:

```text
# HEADER
# Program Title: Inverse hyperbolic functions
# Program No.  : A-2
# DEG/RAD      : Arbitrary
# DPS          : 4
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

Transcribe the program table from `program-table.png`, not from OCR alone.
Translate Sanyo key names into the Santron source vocabulary already used in
existing `.sce` files. Examples seen so far:

- `SM n` -> `STO n`
- `RM n` -> `RCL n`
- `R/S` stays `R/S`
- skipped/padding program slots are `SST` in source and `099` in listings
- `PS n` sets decimal places in RUN mode
- addresses are checked through `GOTO d d`

Preserve original program behavior even if it exposes emulator gaps. If a page
only works after adding a key that is not in the printed table, investigate the
core model first. A-2 showed that `=` must evaluate open parentheses rather than
requiring an artificial `)`.

The translation rule applies only to the key names themselves. Do not rewrite
the surrounding prose into a cleaner explanation. The repository source should
follow the booklet's structure and wording, except where a graphical formula
must be rendered as text.

If a symbol is unclear, inspect `page.png` or `program-table.png` instead of
guessing from OCR.

## Verify The Listing

First inspect the generated listing:

```bash
node santron-cli.js --scenario-file src/sanyo-cz-0911pg/mathematics/inverse-hyperbolic-functions.sce --scenario "list"
```

Check:

- program starts at the printed address, usually `00`
- alternate entry points match the table, such as `20` and `40`
- numeric key codes match the strict expected listing
- the program is padded through address `71`

Save the strict listing under:

```text
bin/sanyo-cz-0911pg/<category>/<slug>.lst
```

The `.lst` is the stable machine-readable representation. It should be exact
and include all addresses through `71`.

Before moving on, read back the created `.sce`, `.lst`, and `.test.sce` once
more against the booklet page. Check that:

- the section order matches the sheet
- the prose is still the booklet prose, not a cleaned-up paraphrase
- only Sanyo key names were mapped to Santron key names
- graphical formulas were converted to text without changing the section layout
- step numbers, addresses, and comments still line up with the program table
- example inputs are actual keypresses, not shorthand that the calculator cannot enter directly

For a quick one-off source check, you can also run:

```bash
node santron-cli.js --scenario-file src/sanyo-cz-0911pg/<category>/<slug>.sce --scenario "list"
node santron-cli.js --scenario-file tests/sanyo-cz-0911pg/<category>/<slug>.test.sce
```

## Add A Regression Test

Create a test under:

```text
tests/sanyo-cz-0911pg/<category>/<slug>.test.sce
```

The test should load the strict listing and run the booklet examples:

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

Use the sheet's `DPS` and `DEG/RAD` requirements. Test all clear example
outputs from the page. If an example is ambiguous, document the ambiguity in
the `.sce` comments and add the test only after visual confirmation.

## Run Checks

For one source while developing:

```bash
node santron-cli.js --scenario-file src/sanyo-cz-0911pg/<category>/<slug>.sce --scenario "list"
node santron-cli.js --scenario-file tests/sanyo-cz-0911pg/<category>/<slug>.test.sce
```

For the project:

```bash
npm run all
```

If core behavior changes, also run the GUI smoke test:

```bash
npm run gui:smoke
```

When the browser model or certificate setup changes, use the service-worker
smoke as well:

```bash
npm run gui:smoke:sw -- https://192.168.2.122:8765
```

## Update The Tracker

After the source, listing, and test are in place, update
`docs/sanyo-library.md`:

- set `src` to `yes` when the `.sce` exists
- set `bin` to `yes` when the strict `.lst` exists
- set `test` to `yes` when a regression scenario exists
- keep category letters and titles aligned with the booklet index
- keep notes for uncertain OCR titles until the page is fully verified

## Practical Notes

Good pages usually cost only a few minutes after preparation. The efficient
loop is:

1. Render the PDF page as PNG.
2. Keep OCR text beside the image.
3. Read only the relevant crops.
4. Write the `.sce` directly.
5. Verify with CLI examples immediately.

Expected context cost from the earlier extraction session:

- good readable page: about 3,000 to 5,000 tokens
- poor OCR, repeated zoom/render attempts, or unclear symbols: about 8,000 to
  12,000 tokens

The main risk is not typing the source; it is silently trusting bad OCR. Always
verify the program table visually and let the original examples drive the final
test.

Be pedantic with the final pass. Re-read the generated files after you write
them, line by line if necessary, and correct anything that is even slightly
off before moving on.

Before committing, run:

```bash
git status
npm test
```

If `santron-core.js` changed, also run:

```bash
node --check santron-core.js
npm run gui:smoke -- https://localhost:8765
```

Do not commit `work/` outputs unless they are explicitly needed.
