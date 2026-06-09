#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const SantronCore = require("./santron-core");

const { KEY_CODES, createCalculator, codeName, isValidProgramCode, keyFromCode, roundInternal } = SantronCore;
const calculator = createCalculator();
const { state } = calculator;
const DEBUG_NUMBER_WIDTH = 18;
const DEBUG_MEMORY_VALUE_WIDTH = 15;
const DEBUG_DISPLAY_WIDTH = 18;
const DEBUG_DISPLAY_VALUE_WIDTH = 13;

function formatHelpColumns(items, columns = 4) {
  const width = Math.max(...items.map((item) => item.length)) + 2;
  const lines = [];
  for (let i = 0; i < items.length; i += columns) {
    lines.push(items.slice(i, i + columns).map((item) => item.padEnd(width, " ")).join("").trimEnd());
  }
  return lines.join("\n");
}

function keyCodeHelp() {
  const items = Object.entries(KEY_CODES)
    .map(([key, code]) => ({ key, code }))
    .sort((left, right) => left.code - right.code || left.key.localeCompare(right.key))
    .map(({ key, code }) => `$${String(code).padStart(3, "0")} ${key}`);
  items.push("$099 blank");
  return formatHelpColumns(items);
}

function usage(exitCode = 0) {
  const script = path.basename(process.argv[1]);
  console.log(`Santron 626 CLI

Usage:
  node ${script} [options]

Options:
  --program <file>          Load .lst program
  --scenario "<script>"     Run key groups and expect checks separated by semicolons
  --scenario-file <file>    Read scenario script from file
  --steps <n>               Maximum program steps after R/S (default: 1000)
  --sst <n>                 Execute n single program steps
  --pc <n>                  Set program counter before scenario
  --dp <n>                  Set display precision 0..7, or 8/9 for auto/scientific mode
  --mode <switches...>      Initial switches, e.g. --mode /DG /RUN /ON
  --trace                  Print each program step while running
  --debug                  Print program step and calculator status before each step
  --mem                    Print memories
  --help                   Show this help

Examples:
  node ${script} --scenario "1 EXP 9 =" --dp 4
  node ${script} --mode /DG /RUN /ON --scenario "1 + 2 =; expect 3."
  node ${script} --program programs/random-number-generator.lst --pc 1 --scenario "0 . 1 2 R/S; expect 0.1039" --steps 200 --dp 4
  node ${script} --program programs/square.lst --scenario "5 R/S; expect 25."
  node ${script} --scenario '/LOAD X^2 R/S; save "mein-programm.lst"'
  node ${script} --scenario 'load "mein-programm.lst"; list'
  node ${script} --scenario '/CLR R/S; load "mein-programm.lst"; /RUN 5 R/S; expect 25.'
  node ${script} --scenario ':LOAD X^2 R/S GOTO 0 0; :RUN GOTO 0 0 2 R/S{4}'
  node ${script} --program programs/mastermind.lst --scenario "1 2 3 4 R/S; expect 1234.21; 5 6 2 1 R/S; expect 5621.12"

Switch tokens:
  /RD /DG /LOAD /CLR /RUN /ON /OFF
  :RD :DG :LOAD :CLR :RUN :ON :OFF

Key tokens:
  Use the textual key name in scenarios, or $nnn for a direct keycode.
  Append {n} to repeat a token, e.g. R/S{100}.

Scenario commands:
  expect <display>          Check current display
  load "file.lst"           Load a program listing
  save "file.lst"           Save current program memory
  list                      Print current program memory in 3 columns

${keyCodeHelp()}
`);
  process.exit(exitCode);
}

function parseArgs(argv) {
  const options = { steps: 1000, sst: 0, trace: false, mem: false };
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    const next = () => {
      i += 1;
      if (i >= argv.length) throw new Error(`${arg} erwartet einen Wert`);
      return argv[i];
    };
    if (arg === "--help" || arg === "-h") usage(0);
    else if (arg === "--program") options.program = next();
    else if (arg === "--scenario") options.scenario = next();
    else if (arg === "--scenario-file") options.scenarioFile = next();
    else if (arg === "--steps") options.steps = Number(next());
    else if (arg === "--sst") options.sst = Number(next());
    else if (arg === "--pc") options.pc = Number(next());
    else if (arg === "--dp" || arg === "--ps") options.dp = Number(next());
    else if (arg === "--mode") {
      options.mode = [];
      while (i + 1 < argv.length && !argv[i + 1].startsWith("--")) {
        i += 1;
        options.mode.push(argv[i]);
      }
      if (!options.mode.length) throw new Error("--mode erwartet mindestens einen Schalter");
    }
    else if (arg === "--trace") options.trace = true;
    else if (arg === "--debug") options.debug = true;
    else if (arg === "--mem") options.mem = true;
    else throw new Error(`Unbekannte Option: ${arg}`);
  }
  return options;
}

function loadProgram(filename) {
  const text = fs.readFileSync(filename, "utf8");
  return calculator.loadProgramListing(text);
}

function saveProgram(filename) {
  fs.writeFileSync(filename, `${calculator.makeProgramListing()}\n`, "utf8");
}

function programListingEntry(code, idx) {
  const address = String(idx).padStart(2, "0");
  const keyCode = String(code).padStart(3, "0");
  const name = codeName(code).padEnd(4, " ");
  return `${address}: ${keyCode} ${name}`;
}

function printProgramList() {
  const rowsPerColumn = 25;
  const columns = [];
  for (let start = 0; start < state.program.length; start += rowsPerColumn) {
    columns.push(state.program.slice(start, start + rowsPerColumn).map((code, offset) => (
      programListingEntry(code, start + offset)
    )));
  }
  const widths = columns.map((column) => Math.max(...column.map((line) => line.length)));
  const rowCount = Math.max(...columns.map((column) => column.length));
  for (let row = 0; row < rowCount; row += 1) {
    console.log(columns.map((column, idx) => (column[row] || "").padEnd(widths[idx])).join("  ").trimEnd());
  }
}

function parseKeys(text) {
  if (!text) return [];
  const aliases = {
    "÷": "/", "DIV": "/", "×": "*", "MUL": "*", "YX": "Y^X", "Y^X": "Y^X",
    "X2": "X^2", "X^2": "X^2", "SQRT": "SQRT", "√X": "SQRT",
    "EE": "EXP", "EXP": "EXP", "ENTER": "=", "RUN": "R/S", "RS": "R/S",
    "R/S": "R/S", "GOTO": "GOTO", "GTO": "GOTO", "CHS": "+/-",
    "MX": "M*", "MDIV": "M/",
    "EX": "E^X", "10X": "10^X",
  };
  const keyNames = new Map(Object.keys(KEY_CODES).map((key) => [key.toUpperCase(), key]));
  const parseOne = (raw) => {
    const codeMatch = raw.match(/^\$(\d{1,3})$/);
    if (codeMatch) return { code: Number(codeMatch[1]) };
    const upper = raw.toUpperCase();
    return aliases[upper] || keyNames.get(upper) || raw;
  };
  return text.split(/[\s,]+/).filter(Boolean).flatMap((raw) => {
    const repeatMatch = raw.match(/^(.+)\{(\d+)\}$/);
    if (!repeatMatch) return [parseOne(raw)];
    const repeat = Number(repeatMatch[2]);
    if (!Number.isInteger(repeat) || repeat < 0) throw new Error(`ungueltige Wiederholung: ${raw}`);
    return Array.from({ length: repeat }, () => parseOne(repeatMatch[1]));
  });
}

function applySwitch(token, options = {}) {
  const normalized = String(token).toUpperCase().replace(/^:/, "/");
  if (normalized === "/RD") {
    state.angle = "rad";
    return true;
  }
  if (normalized === "/DG") {
    state.angle = "deg";
    return true;
  }
  if (normalized === "/LOAD") {
    state.mode = "load";
    calculator.stopProgram();
    state.pendingGotoDigits = null;
    state.gotoPreview = null;
    state.pendingPrecision = false;
    return true;
  }
  if (normalized === "/CLR") {
    state.mode = "clear";
    calculator.stopProgram();
    state.pendingGotoDigits = null;
    state.gotoPreview = null;
    state.pendingPrecision = false;
    return true;
  }
  if (normalized === "/RUN") {
    state.mode = "run";
    return true;
  }
  if (normalized === "/ON") {
    if (!options.initial && state.power !== "on") calculator.resetCalculatorState();
    state.power = "on";
    return true;
  }
  if (normalized === "/OFF") {
    calculator.stopProgram();
    state.power = "off";
    return true;
  }
  return false;
}

function isSwitchToken(token) {
  const normalized = String(token).toUpperCase().replace(/^:/, "/");
  return ["/RD", "/DG", "/LOAD", "/CLR", "/RUN", "/ON", "/OFF"].includes(normalized);
}

function executeKeyToken(token) {
  if (typeof token === "string" && isSwitchToken(token)) {
    if (!applySwitch(token)) throw new Error(`Unbekannter Schalter: ${token}`);
    return;
  }
  if (typeof token === "object" && token !== null && "code" in token) {
    if (token.code === 99) return;
    if (!isValidProgramCode(token.code)) throw new Error(`Keycode ${token.code} ist unbekannt`);
    const key = keyFromCode(token.code);
    if (!key) return;
    calculator.execute(key);
    return;
  }
  calculator.execute(token);
}

function isRunStopToken(token) {
  return typeof token === "string" && String(token).toUpperCase() === "R/S";
}

function traceProgramStep({ steps: stepNo, pc, code, name, display }) {
  console.log(`${String(stepNo).padStart(4, " ")}  ${String(pc).padStart(2, "0")}  ${String(code).padStart(3, "0")}  ${(name || codeName(code) || "?").padEnd(5)}  ${display.trim()}`);
}

let debugRowsPrinted = 0;

function formatParenStack(parenStack) {
  if (!parenStack.length) return "[]";
  return parenStack
    .map((entry) => `${entry.y}${entry.pending || ""}(`)
    .join(" ");
}

function debugCell(value, width) {
  const text = String(value ?? "");
  return text.length >= width ? text : text.padEnd(width, " ");
}

function debugCellRight(value, width) {
  const text = String(value ?? "");
  return text.length >= width ? text : text.padStart(width, " ");
}

function debugHeader() {
  const columns = [
    debugCell("step", 5),
    debugCell("pc", 3),
    debugCell("cod", 3),
    debugCell("key", 4),
    debugCell("ang/m", 5),
    "|",
    debugCellRight("display", DEBUG_DISPLAY_WIDTH),
    "|",
    debugCellRight("x", DEBUG_NUMBER_WIDTH),
    "|",
    debugCellRight("y", DEBUG_NUMBER_WIDTH),
    "|",
    debugCell("pending", 8),
    "parenStack",
  ];
  return columns.join(" ");
}

function modeShortName(mode) {
  if (mode === "load") return "L";
  if (mode === "clear") return "C";
  return "R";
}

function angleModeText(angle, mode) {
  return `${angle === "rad" ? "RD" : "DG"}/${modeShortName(mode)}`;
}

function statusLine(steps) {
  const columns = [
    debugCell(String(steps ?? 0).padStart(4, "0"), 5),
    debugCell(String(state.pc).padStart(2, "0"), 3),
    debugCell("---", 3),
    debugCell("---", 4),
    debugCellRight(angleModeText(state.angle, state.mode), 5),
    "|",
    debugCellRight(formatDebugDisplay(calculator.displayText()), DEBUG_DISPLAY_WIDTH),
    "|",
    debugCellRight(formatDebugNumber(state.x), DEBUG_NUMBER_WIDTH),
    "|",
    debugCellRight(formatDebugNumber(state.y), DEBUG_NUMBER_WIDTH),
    "|",
    debugCell(state.pending || "", 8),
    formatParenStack(state.parenStack),
  ];
  return columns.join(" ");
}

function formatDebugNumber(value) {
  const number = Number(value);
  if (!Number.isFinite(number)) return String(value);
  const rounded = roundInternal(number);
  if (Object.is(rounded, -0) || rounded === 0) return "0";
  const normal = String(Number(rounded.toPrecision(10)));
  if (!normal.includes("e") && normal.length <= 15) return normal;

  const sign = rounded < 0 ? "-" : " ";
  const abs = Math.abs(rounded);
  const exponent = Math.floor(Math.log10(abs));
  const mantissa = abs / (10 ** exponent);
  const scaledMantissa = Math.trunc((mantissa * 1e9) + (Number.EPSILON * 16));
  const digits = String(scaledMantissa).padStart(10, "0");
  const exponentSign = exponent < 0 ? "-" : " ";
  const exponentDigits = String(Math.abs(exponent)).padStart(2, "0").slice(-2);
  return `${sign}${digits[0]}.${digits.slice(1)}${exponentSign}${exponentDigits}`;
}

function formatDebugDisplay(value) {
  const text = String(value ?? "").trim();
  return text.length > DEBUG_DISPLAY_VALUE_WIDTH ? text.slice(0, DEBUG_DISPLAY_VALUE_WIDTH) : text;
}

function formatDebugMemories(memories) {
  const formatRange = (start, end) => memories
    .slice(start, end)
    .map((value, offset) => `M${start + offset}:${debugCellRight(formatDebugNumber(value), DEBUG_MEMORY_VALUE_WIDTH)}`)
    .join(" | ");
  return [
    `      ${formatRange(0, 5)}`,
    `      ${formatRange(5, 10)}`,
  ].join("\n");
}

function debugProgramStep({ steps: stepNo, pc, code, name, before }) {
  if (debugRowsPrinted % 10 === 0) console.log(`\n${debugHeader()}`);
  debugRowsPrinted += 1;
  const columns = [
    debugCell(String(stepNo).padStart(4, "0"), 5),
    debugCell(String(pc).padStart(2, "0"), 3),
    debugCell(String(code).padStart(3, "0"), 3),
    debugCell(name || codeName(code) || "?", 4),
    debugCellRight(angleModeText(before.angle, before.mode), 5),
    "|",
    debugCellRight(formatDebugDisplay(before.display), DEBUG_DISPLAY_WIDTH),
    "|",
    debugCellRight(formatDebugNumber(before.x), DEBUG_NUMBER_WIDTH),
    "|",
    debugCellRight(formatDebugNumber(before.y), DEBUG_NUMBER_WIDTH),
    "|",
    debugCell(before.pending || "", 8),
    formatParenStack(before.parenStack),
  ];
  console.log(columns.join(" "));
  console.log(formatDebugMemories(before.memories));
}

function programStepObserver(options) {
  if (!options.trace && !options.debug) return null;
  return (step) => {
    const adjustedStep = { ...step, steps: (options.stepOffset || 0) + step.steps };
    if (options.debug) debugProgramStep(adjustedStep);
    if (options.trace) traceProgramStep(adjustedStep);
  };
}

function runUntilStop(options) {
  return calculator.runProgram(options.steps, programStepObserver(options));
}

function printRunStopDisplay(options) {
  options.runStopCount = (options.runStopCount || 0) + 1;
  console.log(`R/S ${String(options.runStopCount).padStart(4, "0")}: ${calculator.displayText().trim()}`);
}

function executeKeyTokenAndMaybeRun(token, options, stepOffset = 0) {
  const wasRunning = state.running;
  const wasRunMode = state.mode === "run";
  const isRunStop = isRunStopToken(token);
  executeKeyToken(token);
  if (!wasRunning && state.running) {
    options.stepOffset = stepOffset;
    let steps;
    try {
      steps = runUntilStop(options);
    } finally {
      options.stepOffset = 0;
    }
    if (state.running) throw new Error(`Programmlauf nach ${options.steps} Schritten noch nicht gestoppt`);
    if (isRunStop && wasRunMode) printRunStopDisplay(options);
    return steps;
  }
  if (isRunStop && wasRunMode) printRunStopDisplay(options);
  return 0;
}

function normalizeScenarioText(text) {
  return text
    .split(/\r?\n/)
    .map((line) => line.replace(/^\s*#.*$/, ""))
    .join("\n");
}

function parseScenarioFileArgument(text) {
  const match = text.match(/^(?:"([^"]+)"|'([^']+)'|(\S+))\s*$/);
  if (!match) return null;
  return match[1] || match[2] || match[3];
}

function scenarioExpectationError(expected, action, actionIndex, steps) {
  const actual = calculator.displayText().trim();
  const lines = [
    `expect failed at scenario step ${actionIndex + 1}`,
    `action: ${action}`,
    `expected display: ${expected}`,
    `actual display:   ${actual}`,
    "",
    debugHeader(),
    statusLine(steps),
    formatDebugMemories(state.memories),
  ];
  if (steps > 0 && state.program.some((code) => code !== 99)) {
    lines.push("", "program:", calculator.makeProgramListing());
  }
  return lines.join("\n");
}

function runScenario(text, options) {
  let steps = 0;
  const actions = normalizeScenarioText(text).split(/[;\r\n]+/).map((part) => part.trim()).filter(Boolean);
  actions.forEach((action, actionIndex) => {
    const expectMatch = action.match(/^expect(?:-display)?\s+(.+)$/i);
    if (expectMatch) {
      const expected = expectMatch[1].trim();
      const actual = calculator.displayText().trim();
      if (actual !== expected) {
        throw new Error(scenarioExpectationError(expected, action, actionIndex, steps));
      }
      return;
    }

    const loadMatch = action.match(/^load\s+(.+)$/i);
    if (loadMatch) {
      const filename = parseScenarioFileArgument(loadMatch[1].trim());
      if (!filename) throw new Error(`ungueltiger load-Befehl: ${action}`);
      loadProgram(filename);
      return;
    }

    const saveMatch = action.match(/^save\s+(.+)$/i);
    if (saveMatch) {
      const filename = parseScenarioFileArgument(saveMatch[1].trim());
      if (!filename) throw new Error(`ungueltiger save-Befehl: ${action}`);
      saveProgram(filename);
      return;
    }

    if (/^list$/i.test(action)) {
      printProgramList();
      return;
    }

    const keyText = action.replace(/^(keys|press)\s+/i, "");
    parseKeys(keyText).forEach((token) => {
      steps += executeKeyTokenAndMaybeRun(token, options, steps);
    });
  });
  return steps;
}

function printSummary(steps) {
  console.log(`\n${debugHeader()}`);
  console.log(statusLine(steps));
  console.log(formatDebugMemories(state.memories));
}

function main() {
  let options;
  try {
    options = parseArgs(process.argv.slice(2));
  } catch (error) {
    console.error(error.message);
    usage(2);
  }

  calculator.resetCalculatorState();
  if (options.program) loadProgram(options.program);
  if (options.mode) options.mode.forEach((token) => {
    if (!applySwitch(token, { initial: true })) throw new Error(`Unbekannter Schalter: ${token}`);
  });
  if (Number.isFinite(options.pc)) state.pc = options.pc;
  if (Number.isFinite(options.dp)) {
    state.pendingPrecision = true;
    calculator.execute(String(options.dp));
  }

  for (let i = 0; i < options.sst; i += 1) calculator.runSingleProgramStep();

  let steps = null;
  try {
    if (options.scenarioFile) {
      const scenarioText = fs.readFileSync(options.scenarioFile, "utf8");
      steps = runScenario(scenarioText, options);
    }
    if (options.scenario) steps = runScenario(options.scenario, options);
  } catch (error) {
    console.error(error.message);
    process.exit(1);
  }

  printSummary(steps);
  if (options.mem) state.memories.forEach((value, idx) => console.log(`M${idx}: ${value}`));
}

main();
