#!/usr/bin/env node

import Core from "../santron-core.js";

const GUI_KEYS = [
  "F", "SIN", "COS", "TAN", "PS",
  "E^X", "LOG", "Y^X", "STO", "RCL",
  "SQRT", "1/X", "X<>Y", "(", ")",
  "EXP", "7", "8", "9", "/",
  "PI", "4", "5", "6", "*",
  "+/-", "1", "2", "3", "+",
  "C/CE", "0", ".", "-", "=",
];

const SHIFT_KEYS = ["ASIN", "ACOS", "ATAN", "LN", "10^X", "X^2", "M+", "M-", "M*", "M/"];
const PROGRAM_ONLY_KEYS = ["R/S", "GOTO", "SKP"];
const ALL_KEYS = [...GUI_KEYS, ...SHIFT_KEYS, ...PROGRAM_ONLY_KEYS];

const failures = [];

for (const key of ALL_KEYS) {
  runCase(`press ${key}`, (calculator) => {
    primeForKey(calculator, key);
    calculator.execute(key);
    assertHealthy(calculator, key);
  });
}

runCase("load mode accepts every program code", (calculator) => {
  calculator.state.mode = "load";
  for (const key of Object.keys(Core.KEY_CODES)) {
    calculator.execute(key);
  }
  assertHealthy(calculator, "load all key codes");
});

runCase("all key codes can be decoded for program execution", (calculator) => {
  for (const code of Object.values(Core.KEY_CODES)) {
    const key = calculator.keyFromCode(code);
    if (!key) throw new Error(`No key for code ${code}`);
  }
});

runCase("R/S preserves a pending manual calculation", (calculator) => {
  press(calculator, "4", "*", "6");
  calculator.execute("R/S");
  assertEqual(calculator.state.y, 4, "R/S changed y");
  assertEqual(calculator.state.x, "6", "R/S changed x");
  assertEqual(calculator.state.pending, "*", "R/S cleared pending");
  calculator.stopProgram();
  calculator.execute("=");
  assertEqual(calculator.state.x, "24", "pending calculation did not resume after R/S");
});

if (failures.length) {
  failures.forEach((failure) => {
    console.error(`FAIL ${failure.name}`);
    console.error(`     ${failure.message}`);
  });
  process.exit(1);
}

console.log(`core-key-coverage: ${ALL_KEYS.length} keys pressed, ${Object.keys(Core.KEY_CODES).length} keycodes checked.`);

function runCase(name, fn) {
  try {
    const calculator = Core.createCalculator();
    fn(calculator);
  } catch (error) {
    failures.push({ name, message: error?.stack || String(error) });
  }
}

function primeForKey(calculator, key) {
  const state = calculator.state;

  if (["SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN"].includes(key)) calculator.execute("1");
  else if (["LN", "LOG", "SQRT"].includes(key)) calculator.execute("4");
  else if (["E^X", "10^X", "X^2", "1/X", "+/-"].includes(key)) calculator.execute("2");
  else if (key === "Y^X") press(calculator, "2");
  else if (["+", "-", "*", "/", "="].includes(key)) press(calculator, "2");
  else if (key === "X<>Y") press(calculator, "2", "+", "3");
  else if (key === ")") press(calculator, "(", "2", "+", "3");
  else if (key === "EXP") press(calculator, "1", "2", "3");
  else if (key === "PS") state.pendingPrecision = false;
  else if (["STO", "RCL", "M+", "M-", "M*", "M/"].includes(key)) press(calculator, "5");
  else if (key === "SKP") calculator.setX(-1);
  else if (key === "R/S") {
    state.program[0] = Core.KEY_CODES["R/S"];
    state.pc = 0;
  } else if (key === "GOTO") {
    state.pc = 12;
  }
}

function press(calculator, ...keys) {
  keys.forEach((key) => calculator.execute(key));
}

function assertEqual(actual, expected, message) {
  if (actual !== expected) {
    throw new Error(`${message}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
  }
}

function assertHealthy(calculator, key) {
  const state = calculator.state;
  const display = calculator.displayText();

  if (typeof display !== "string" || display.length === 0) {
    throw new Error(`${key}: displayText returned invalid value ${JSON.stringify(display)}`);
  }
  if (state.pc < 0 || state.pc > Core.PROGRAM_SIZE) {
    throw new Error(`${key}: pc out of range: ${state.pc}`);
  }
  if (!Array.isArray(state.program) || state.program.length !== Core.PROGRAM_SIZE) {
    throw new Error(`${key}: program has invalid length`);
  }
  if (!Array.isArray(state.memories) || state.memories.length !== 10) {
    throw new Error(`${key}: memories has invalid length`);
  }
  if (!state.memories.every((value) => Number.isFinite(Number(value)))) {
    throw new Error(`${key}: memory contains non-finite value`);
  }
  if (state.pendingGotoDigits !== null && !Array.isArray(state.pendingGotoDigits)) {
    throw new Error(`${key}: pendingGotoDigits has invalid type`);
  }
  if (state.pendingMemory !== null && !["STO", "RCL", "M+", "M-", "M*", "M/"].includes(state.pendingMemory)) {
    throw new Error(`${key}: pendingMemory has invalid value ${state.pendingMemory}`);
  }
}
