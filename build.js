#!/usr/bin/env node
"use strict";

// build.js — kompiliert alle src/**/*.sce zu bin/**/*.lst
//
// Konvention:
//   src/<namespace>/<category>/<name>.sce  →  bin/<namespace>/<category>/<name>.lst
//
// Das Szenario darf keinen "save"-Befehl enthalten; build.js übernimmt das Speichern.
// Nach dem /load-Block muss der Rechner im LOAD- oder CLR-Modus stehen bleiben
// (kein /run am Ende der Quelldatei).

const fs = require("fs");
const path = require("path");
const { SantronRunner } = require("./santron-cli");

const SRC_DIR = path.join(__dirname, "src");
const BIN_DIR = path.join(__dirname, "bin");

function findSceFiles(dir) {
  const results = [];
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      results.push(...findSceFiles(full));
    } else if (entry.isFile() && entry.name.endsWith(".sce")) {
      results.push(full);
    }
  }
  return results;
}

function srcToOut(srcFile) {
  const rel = path.relative(SRC_DIR, srcFile);           // sanyo-cz-0911pg/mathematics/foo.sce
  const lstRel = rel.replace(/\.sce$/, ".lst");           // sanyo-cz-0911pg/mathematics/foo.lst
  return path.join(BIN_DIR, lstRel);
}

let built = 0;
let failed = 0;

const sceFiles = findSceFiles(SRC_DIR);

if (sceFiles.length === 0) {
  console.log("build: keine .sce-Dateien in src/ gefunden.");
  process.exit(0);
}

for (const srcFile of sceFiles) {
  const outFile = srcToOut(srcFile);
  const rel = path.relative(__dirname, srcFile);
  const outRel = path.relative(__dirname, outFile);

  try {
    const scenarioText = fs.readFileSync(srcFile, "utf8");
    const runner = new SantronRunner();
    runner.runScenario(scenarioText, { steps: 10000 });

    fs.mkdirSync(path.dirname(outFile), { recursive: true });
    runner.saveProgram(outFile);

    console.log(`  OK  ${rel}  →  ${outRel}`);
    built += 1;
  } catch (err) {
    console.error(`FAIL  ${rel}`);
    console.error(`      ${err.message}`);
    failed += 1;
  }
}

console.log(`\nbuild: ${built} gebaut, ${failed} fehlgeschlagen.`);
process.exit(failed > 0 ? 1 : 0);
