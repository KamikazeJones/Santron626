# page-006

Prepared from:

```text
/mnt/c/Users/quass/Documents/Calculator/sanyo-ocr/page-006_ocr.pdf
```

Files:

- `page.txt`: layout-preserving text from `pdftotext -layout`
- `page.png`: rendered page at 300 dpi
- `program-table.png`: right-side program table crop
- `main-text.png`: title/formula crop
- `examples.png`: examples crop
- `operation-notes.png`: operation and notes crop
- `draft.sce`: initial source template with raw OCR text

Next steps:

1. Read `page.png` and `program-table.png`.
2. Fill header, formula, examples, operation, notes, and data memory in `draft.sce`.
3. Replace the source-code TODO with textual key names.
4. Verify with:

```bash
node santron-cli.js --scenario-file "work/sanyo-pages/page-006/draft.sce" --scenario "list"
```
