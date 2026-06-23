#!/usr/bin/env node
"use strict";

const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync, spawnSync } = require("child_process");
const SantronCore = require("../santron-core");

const repoRoot = path.resolve(__dirname, "..");
const binary = path.join(os.tmpdir(), "santron-vm-check");
const K = SantronCore.KEY_CODES;

function compileVm() {
  execFileSync("cc", [
    "-std=c11",
    "-O2",
    "-Wall",
    "-Wextra",
    path.join(repoRoot, "tools", "santron-vm.c"),
    "-lm",
    "-o",
    binary,
  ], { stdio: "inherit" });
}

function listingFromCodes(codes) {
  const program = Array(72).fill(K["R/S"] + 9);
  codes.forEach((code, idx) => {
    program[idx] = code;
  });
  return program
    .map((code, idx) => `${String(idx).padStart(2, "0")} ${String(code).padStart(3, "0")}`)
    .join("\n");
}

function listingFromAddressMap(entries, size = 100) {
  const program = Array(size).fill(K["R/S"] + 9);
  for (const [idx, code] of entries) program[idx] = code;
  return program
    .map((code, idx) => `${String(idx).padStart(2, "0")} ${String(code).padStart(3, "0")}`)
    .join("\n");
}

function parseDump(output) {
  const lines = output.trim().split(/\r?\n/);
  const first = lines[0].match(/^pc=(\d+)\s+x=([^\s]+)\s+y=([^\s]+).*pending=(\d+)/);
  if (!first) throw new Error(`cannot parse C dump:\n${output}`);

  const state = {
    pc: Number(first[1]),
    x: Number(first[2]),
    y: Number(first[3]),
    pending: Number(first[4]),
    memory: Array(10).fill(0),
  };

  const memLine = lines.find((line) => line.startsWith("M0="));
  if (!memLine) return state;
  for (const match of memLine.matchAll(/M(\d)=([^\s]+)/g)) {
    state.memory[Number(match[1])] = Number(match[2]);
  }
  return state;
}

function runCWithListing(listing, args) {
  const file = path.join(os.tmpdir(), `santron-vm-check-${process.pid}-${Math.random().toString(16).slice(2)}.lst`);
  fs.writeFileSync(file, listing);
  try {
    return runCBinary(["--program", file, ...args, "--dump"]);
  } finally {
    fs.rmSync(file, { force: true });
  }
}

function runCBinary(args) {
  const result = spawnSync(binary, args, { encoding: "utf8" });
  if (result.status !== 0) {
    throw new Error(`C VM failed with status ${result.status}:\n${result.stderr}${result.stdout}`);
  }
  if (result.error && !result.stdout) throw result.error;
  return parseDump(result.stdout);
}

function runJSWithCodes(codes, setup) {
  const calculator = SantronCore.createCalculator();
  calculator.state.program = Array(72).fill(99);
  codes.forEach((code, idx) => {
    calculator.state.program[idx] = code;
  });
  if (setup) setup(calculator.state, calculator);
  calculator.runProgram(2000);
  return {
    pc: calculator.state.pc,
    x: Number(calculator.state.x),
    y: calculator.state.y,
    pending: calculator.state.pending ? 1 : 0,
    memory: calculator.state.memories.slice(),
  };
}

function runCWithProgramFile(programFile, args) {
  return runCBinary(["--program", programFile, ...args, "--dump"]);
}

function runJSMastermind(keys) {
  const calculator = SantronCore.createCalculator();
  calculator.loadProgramListing(fs.readFileSync(path.join(repoRoot, "mastermind-11.lst"), "utf8"));
  calculator.state.memories[0] = 1;
  calculator.state.memories[1] = 0;
  calculator.state.memories[2] = 1;
  calculator.state.memories[8] = 3040210;
  calculator.state.memories[9] = 4;

  calculator.runProgram(2000);
  for (const key of keys) {
    calculator.execute(String(key));
    calculator.runProgram(2000);
  }

  return {
    pc: calculator.state.pc,
    x: Number(calculator.state.x),
    y: calculator.state.y,
    pending: calculator.state.pending ? 1 : 0,
    memory: calculator.state.memories.slice(),
  };
}

function assertClose(name, field, cValue, jsValue, tolerance = 1e-7) {
  if (Number.isNaN(cValue) && Number.isNaN(jsValue)) return;
  if (Math.abs(cValue - jsValue) <= tolerance) return;
  throw new Error(`${name}: ${field} differs: C=${cValue} JS=${jsValue}`);
}

function compareState(name, cState, jsState, memoryIndexes = []) {
  assertClose(name, "x", cState.x, jsState.x);
  assertClose(name, "y", cState.y, jsState.y);
  for (const idx of memoryIndexes) {
    assertClose(name, `M${idx}`, cState.memory[idx], jsState.memory[idx]);
  }
  console.log(`PASS ${name}`);
}

function codeNames(names) {
  return names.map((name) => /^[0-9]$/.test(name) ? K[name] : K[name]);
}

compileVm();

const cases = [
  {
    name: "decimal EXP",
    codes: codeNames(["1", ".", "5", "EXP", "2", "=", "R/S"]),
  },
  {
    name: "parentheses",
    codes: codeNames(["(", "2", "+", "3", ")", "*", "4", "=", "R/S"]),
  },
  {
    name: "trig deg",
    codes: codeNames(["3", "0", "SIN", "R/S"]),
  },
  {
    name: "log tenx",
    codes: codeNames(["1", "0", "0", "LOG", "10^X", "R/S"]),
  },
  {
    name: "memory",
    codes: codeNames(["2", "STO", "5", "3", "M+", "5", "RCL", "5", "R/S"]),
    memory: [5],
  },
  {
    name: "skp skips goto",
    codes: codeNames(["1", "+/-", "SKP", "GOTO", "0", "8", "9", "R/S", "4", "R/S"]),
  },
];

for (const test of cases) {
  const listing = listingFromCodes(test.codes);
  const cState = runCWithListing(listing, ["--run", "--steps", "2000"]);
  const jsState = runJSWithCodes(test.codes);
  compareState(test.name, cState, jsState, test.memory || []);
}

{
  const listing = listingFromAddressMap([
    [0, K["GOTO"]],
    [1, K["9"]],
    [2, K["9"]],
    [99, K["R/S"]],
  ]);
  const cState = runCWithListing(listing, ["--run", "--steps", "2000"]);
  if (cState.pc !== 100) throw new Error(`C VM 100-cell goto failed: pc=${cState.pc}`);
  console.log("PASS c-vm goto 99");
}

const cMastermind = runCWithProgramFile(path.join(repoRoot, "mastermind-11.lst"), [
  "--mem", "M0=1",
  "--mem", "M1=0",
  "--mem", "M2=1",
  "--mem", "M8=3040210",
  "--mem", "M9=4",
  "--run",
  "--keys", "3,5,1,4",
  "--steps", "2000",
]);
compareState("mastermind 3514", cMastermind, runJSMastermind([3, 5, 1, 4]), [0, 1, 2, 4, 7, 8, 9]);

const cMastermindExact = runCWithProgramFile(path.join(repoRoot, "mastermind-11.lst"), [
  "--mem", "M0=1",
  "--mem", "M1=0",
  "--mem", "M2=1",
  "--mem", "M8=3040210",
  "--mem", "M9=4",
  "--run",
  "--keys", "1,2,6,4",
  "--steps", "2000",
]);
compareState("mastermind 1264", cMastermindExact, runJSMastermind([1, 2, 6, 4]), [0, 1, 2, 4, 7, 8, 9]);
