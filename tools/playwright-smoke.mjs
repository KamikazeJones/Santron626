#!/usr/bin/env node

import { chromium } from "playwright";

const SEGMENTS = {
  "0": "abcdef",
  "1": "bc",
  "2": "abged",
  "3": "abgcd",
  "4": "fgbc",
  "5": "afgcd",
  "6": "afgecd",
  "7": "abc",
  "8": "abcdefg",
  "9": "abfgcd",
  "-": "g",
  " ": "",
  "E": "afged",
  "r": "eg",
  "o": "egcd",
  "L": "fed",
  "d": "bgecd",
  "P": "abfge",
  "S": "afgcd",
  "C": "afed",
};

const SEGMENT_NAMES = "abcdefg".split("");
const CHAR_BY_SEGMENTS = new Map(
  Object.entries(SEGMENTS).map(([char, segments]) => [normalizeSegments(segments), char]),
);

const url = process.argv[2] || "https://localhost:8765";
const browser = await chromium.launch();
const context = await browser.newContext({
  ignoreHTTPSErrors: true,
  serviceWorkers: "block",
});
const page = await context.newPage();

const errors = [];
page.on("pageerror", (error) => errors.push(error.message));
page.on("console", (message) => {
  if (message.type() === "error") errors.push(message.text());
});

await page.goto(url, { waitUntil: "networkidle" });
await page.locator("#display").waitFor();
const keyCount = await page.locator("[data-key]").count();
const initialDisplay = await readSegmentDisplay(page);

for (const key of ["1", "+", "2", "="]) {
  await pressKey(page, key);
}
const calculatedDisplay = await readSegmentDisplay(page);

for (const key of ["C/CE", "1", "2", "3", "EXP"]) {
  await pressKey(page, key);
}
const exponentEntryDisplay = await readSegmentDisplay(page);

await context.close();
await browser.close();

if (errors.length) {
  console.error(errors.join("\n"));
  process.exit(1);
}

console.log(`Loaded ${url}`);
console.log(`Keys: ${keyCount}`);
console.log(`Initial display: ${initialDisplay}`);
console.log(`After 1 + 2 =: ${calculatedDisplay}`);
console.log(`After 123 EXP: ${exponentEntryDisplay}`);

if (keyCount !== 40) {
  console.error(`Expected 40 keys, got ${keyCount}`);
  process.exit(1);
}

if (initialDisplay.trim() !== "0.") {
  console.error(`Expected initial display "0.", got "${initialDisplay.trim()}"`);
  process.exit(1);
}

if (calculatedDisplay.trim() !== "3.") {
  console.error(`Expected display "3." after 1 + 2 =, got "${calculatedDisplay.trim()}"`);
  process.exit(1);
}

if (exponentEntryDisplay.trim() !== "123. 00") {
  console.error(`Expected display "123. 00" after 123 EXP, got "${exponentEntryDisplay.trim()}"`);
  process.exit(1);
}

async function pressKey(page, key) {
  await page.locator(`[data-key="${cssEscape(key)}"]`).click();
}

async function readSegmentDisplay(page) {
  const cells = await page.locator("#display .digit").evaluateAll((digits, segmentNames) => digits.map((digit) => {
    const segments = segmentNames
      .filter((name) => digit.querySelector(`.seg.${name}`)?.classList.contains("on"))
      .join("");
    const dot = digit.querySelector(".dot")?.classList.contains("on") || false;
    return { segments, dot };
  }), SEGMENT_NAMES);

  return cells.map(({ segments, dot }) => {
    const char = CHAR_BY_SEGMENTS.get(normalizeSegments(segments)) ?? "?";
    return `${char}${dot ? "." : ""}`;
  }).join("");
}

function normalizeSegments(segments) {
  return [...segments].sort().join("");
}

function cssEscape(value) {
  return String(value).replace(/["\\]/g, "\\$&");
}
