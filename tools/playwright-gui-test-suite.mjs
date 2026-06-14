#!/usr/bin/env node

import fs from "fs";
import path from "path";
import { spawnSync } from "child_process";

const ROOT_DIR = path.resolve(path.dirname(new URL(import.meta.url).pathname), "..");
const TESTS_DIR = path.join(ROOT_DIR, "tests");
const RUNNER = path.join(ROOT_DIR, "tools", "playwright-gui-test.mjs");
const DEFAULT_GUI_URL = "https://localhost:8765";

function usage(exitCode = 0) {
  console.log(`GUI repository test suite

Usage:
  node tools/playwright-gui-test-suite.mjs [<url>]
  node tools/playwright-gui-test-suite.mjs --help

Notes:
  Runs every tests/**/*.test.sce through the GUI runner in repository mode.
  If no URL is given, ${DEFAULT_GUI_URL} is used.
`);
  process.exit(exitCode);
}

function findTestFiles(dir) {
  const results = [];
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) results.push(...findTestFiles(full));
    else if (entry.isFile() && entry.name.endsWith(".test.sce")) results.push(full);
  }
  return results.sort();
}

const args = process.argv.slice(2);
if (args.includes("--help") || args.includes("-h")) usage(0);
if (args.length > 1) usage(2);

const guiUrl = args[0] || DEFAULT_GUI_URL;
const tests = findTestFiles(TESTS_DIR);

if (!tests.length) {
  console.log("gui-test-suite: keine .test.sce-Dateien in tests/ gefunden.");
  process.exit(0);
}

let passed = 0;
let failed = 0;

for (const testFile of tests) {
  const rel = path.relative(ROOT_DIR, testFile);
  const result = spawnSync(process.execPath, [RUNNER, "--repository", guiUrl, testFile], {
    cwd: ROOT_DIR,
    encoding: "utf8",
  });

  if (result.stdout) process.stdout.write(result.stdout);
  if (result.stderr) process.stderr.write(result.stderr);

  if (result.status === 0) passed += 1;
  else failed += 1;

  if (result.status !== 0) {
    console.error(`FAIL  ${rel}`);
  }
}

console.log(`\ngui-test-suite: ${passed} bestanden, ${failed} fehlgeschlagen.`);
process.exit(failed > 0 ? 1 : 0);
