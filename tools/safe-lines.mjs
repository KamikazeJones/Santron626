#!/usr/bin/env node

import fs from "node:fs";

function usage() {
  console.error("Usage: node tools/safe-lines.mjs <file> [start] [count]");
  process.exit(2);
}

const [, , file, startArg = "1", countArg = "40"] = process.argv;
if (!file) usage();

const start = Number(startArg);
const count = Number(countArg);
if (!Number.isInteger(start) || start < 1 || !Number.isInteger(count) || count < 1) usage();

const text = fs.readFileSync(file, "utf8");
const lines = text.split(/\r?\n/);
const end = Math.min(lines.length, start + count - 1);
const width = String(end).length;

function escapeForTerminal(line) {
  return [...line].map((ch) => {
    const code = ch.codePointAt(0);
    if (code === 9) return "\\t";
    if (code >= 32 && code <= 126) return ch;
    if (code <= 0xffff) return `\\u${code.toString(16).padStart(4, "0")}`;
    return `\\u{${code.toString(16)}}`;
  }).join("");
}

for (let i = start; i <= end; i += 1) {
  console.log(`${String(i).padStart(width, " ")}: ${escapeForTerminal(lines[i - 1])}`);
}
