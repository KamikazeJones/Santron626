#!/usr/bin/env node

import fs from "node:fs";

function usage() {
  console.error("Usage: node tools/replace-range.mjs <file> <start> <end> <replacement-file>");
  process.exit(2);
}

const [, , file, startArg, endArg, replacementFile] = process.argv;
if (!file || !startArg || !endArg || !replacementFile) usage();

const start = Number(startArg);
const end = Number(endArg);
if (!Number.isInteger(start) || start < 1 || !Number.isInteger(end) || end < start) usage();

const original = fs.readFileSync(file, "utf8");
const newline = original.includes("\r\n") ? "\r\n" : "\n";
const lines = original.split(/\r?\n/);
if (end > lines.length) {
  console.error(`Range ends at ${end}, but file has only ${lines.length} lines.`);
  process.exit(1);
}

const replacement = fs.readFileSync(replacementFile, "utf8").replace(/\r?\n$/, "");
const replacementLines = replacement.length ? replacement.split(/\r?\n/) : [];
const updated = [
  ...lines.slice(0, start - 1),
  ...replacementLines,
  ...lines.slice(end),
].join(newline);

fs.writeFileSync(file, updated, "utf8");
