#!/usr/bin/env node
"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const core = require("../santron-core.js");
const root = path.resolve(__dirname, "..");
const listing = fs.readFileSync(path.join(root, "mastermind-20.lst"), "utf8");
const program = core.parseProgramListing(listing).program;
const inputPc = program.indexOf(core.KEY_CODES["R/S"]) + 1;
const resultPc = program.lastIndexOf(core.KEY_CODES["R/S"]) + 1;

for (const file of ["programs/mastermind-20.lst", "mastermind-20-kommentiert.lst"]) {
  assert.deepEqual(core.parseProgramListing(fs.readFileSync(path.join(root, file), "utf8")).program, program, file);
}
assert.equal(program.filter(code => code !== 99).length, 72);

const codes = [];
for (let a = 1; a <= 6; a++) for (let b = 1; b <= 6; b++)
  for (let c = 1; c <= 6; c++) for (let d = 1; d <= 6; d++) {
    const digits = [a, b, c, d];
    if (new Set(digits).size === 4) codes.push(digits);
  }

function resume(calc) {
  calc.execute("R/S");
  let steps = 0;
  while (calc.state.running && steps++ < 1000) calc.runOneProgramStep();
  assert.equal(calc.state.running, false, "program exceeded step limit");
  assert.equal(calc.state.programPaused, true, "program must halt at R/S");
}

function startRound(calc) {
  // Deliberately spoil working registers before resuming the result halt.
  calc.state.memories[0] = -17;
  calc.state.memories[2] = 99;
  calc.state.memories[4] = -123;
  calc.state.memories[9] = 999;
  resume(calc);
  assert.equal(calc.state.pc, inputPc);
  assert.equal(calc.state.memories[0], -17);
  assert.equal(calc.state.memories[2], 1);
  assert.equal(calc.state.memories[9], 36);
  assert.equal(calc.displayText().trim(), "1.");
}

let cases = 0;
const calc = core.createCalculator();
calc.loadProgramListing(listing);
calc.state.memories[1] = 321;
calc.state.memories[7] = -456;
for (const code of codes) {
  const encoded = code.reduce((sum, digit, index) => sum + (index + 1) * 10 ** digit, 0);
  // Changing M8 at the result halt also exercises switching the secret code.
  calc.state.memories[8] = encoded;
  for (const guess of codes) {
    startRound(calc);
    for (const [index, digit] of guess.entries()) {
      calc.execute(String(digit));
      resume(calc);
      if (index < 3) {
        assert.equal(calc.state.pc, inputPc);
        assert.equal(calc.displayText().trim(), `${index + 2}.`);
      }
    }
    const black = guess.filter((digit, index) => digit === code[index]).length;
    const white = guess.filter(digit => code.includes(digit)).length - black;
    const context = `code=${code.join("")} guess=${guess.join("")}`;
    assert.equal(calc.state.pc, resultPc, context);
    assert.ok(Math.abs(Number(calc.state.x) - (black + white / 10)) < 1e-9, context);
    assert.equal(calc.displayText().trim(), `${black}.${white}`, context);
    assert.equal(calc.state.memories[9], 10 * black + white, context);
    assert.equal(calc.state.memories[8], encoded, context);
    assert.equal(calc.state.memories[1], 321, context);
    assert.equal(calc.state.memories[7], -456, context);
    // Reading the accumulator at the halt must not affect the next round.
    calc.execute("RCL");
    calc.execute("9");
    cases++;
  }
}
console.log(`PASS ${cases} code/guess combinations: input positions, automatic results, restart, code changes and matching listings.`);
