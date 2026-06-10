#!/usr/bin/env node
"use strict";

// test.js — führt alle tests/**/*.test.sce aus
//
// Konvention:
//   tests/<namespace>/<category>/<name>.test.sce
//
// Eine .test.sce-Datei lädt das zugehörige .lst aus bin/ und prüft
// Berechnungsergebnisse mit "expect"-Befehlen.
//
// Ausgabe:
//   PASS  tests/sanyo-cz-0911pg/mathematics/foo.test.sce
//   FAIL  tests/sanyo-cz-0911pg/mathematics/bar.test.sce
//         expect failed at scenario step 3 ...

const fs = require("fs");
const path = require("path");
const { SantronRunner } = require("./santron-cli");

const TESTS_DIR = path.join(__dirname, "tests");

function findTestFiles(dir) {
  const results = [];
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      results.push(...findTestFiles(full));
    } else if (entry.isFile() && entry.name.endsWith(".test.sce")) {
      results.push(full);
    }
  }
  return results;
}

let passed = 0;
let failed = 0;

const testFiles = findTestFiles(TESTS_DIR);

if (testFiles.length === 0) {
  console.log("test: keine .test.sce-Dateien in tests/ gefunden.");
  process.exit(0);
}

for (const testFile of testFiles) {
  const rel = path.relative(__dirname, testFile);
  try {
    const scenarioText = fs.readFileSync(testFile, "utf8");
    const runner = new SantronRunner();
    runner.runScenario(scenarioText, { steps: 10000 });
    console.log(`PASS  ${rel}`);
    passed += 1;
  } catch (err) {
    console.error(`FAIL  ${rel}`);
    err.message.split("\n").forEach((line) => console.error(`      ${line}`));
    failed += 1;
  }
}

console.log(`\ntest: ${passed} bestanden, ${failed} fehlgeschlagen.`);
process.exit(failed > 0 ? 1 : 0);
