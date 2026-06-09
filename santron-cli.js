#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const SantronCore = require("./santron-core");

const { KEY_CODES, createCalculator, codeName, isValidProgramCode, keyFromCode, roundInternal } = SantronCore;
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
  --key-trace              Print each key/switch token as it is executed
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
  node ${script} --scenario '/LOAD %loop X^2 R/S GOTO &loop; /RUN 3 R/S'
  node ${script} --scenario '/LOAD GOTO &end X^2 %end =; list'
  node ${script} --program programs/mastermind.lst --scenario "1 2 3 4 R/S; expect 1234.21; 5 6 2 1 R/S; expect 5621.12"

Label syntax (2-pass):
  %name    Defines a label at the current program address (only in /LOAD mode)
  &name    References a label – replaced by the two-digit address (e.g. 2 7 for address 27)
           In /LOAD mode: two placeholder cells (pass 1), patched in pass 2
           In /RUN mode:  direct replacement by the address digits

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
    else if (arg === "--key-trace") options.keyTrace = true;
    else if (arg === "--debug") options.debug = true;
    else if (arg === "--mem") options.mem = true;
    else throw new Error(`Unbekannte Option: ${arg}`);
  }
  return options;
}

function programListingEntry(code, idx) {
  const address = String(idx).padStart(2, "0");
  const keyCode = String(code).padStart(3, "0");
  const name = codeName(code).padEnd(4, " ");
  return `${address}: ${keyCode} ${name}`;
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
    if (raw.startsWith('%') && raw.length > 1) return { label: raw.slice(1) };
    if (raw.startsWith('&') && raw.length > 1) return { ref: raw.slice(1) };
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

function simulateModeAndPc(tokens, pc, mode, labelMap) {
  for (const token of tokens) {
    if (token === null || token === undefined) continue;
    if (typeof token === "object") {
      if ("label" in token) {
        if (mode === "load" || mode === "clear") labelMap[token.label] = pc;
        continue;
      }
      if ("ref" in token) {
        if (mode === "load" || mode === "clear") pc += 2;
        continue;
      }
      if (mode === "load" || mode === "clear") pc += 1;
      continue;
    }
    if (typeof token === "string") {
      if (isSwitchToken(token)) {
        const norm = token.toUpperCase().replace(/^:/, "/");
        if (norm === "/LOAD") mode = "load";
        else if (norm === "/CLR") mode = "clear";
        else if (norm === "/RUN") mode = "run";
        continue;
      }
      if (mode === "load" || mode === "clear") pc += 1;
    }
  }
  return { pc, mode };
}

function buildLabelMap(actions, startPc, startMode) {
  const labelMap = {};
  let pc = startPc;
  let mode = startMode;
  for (const action of actions) {
    if (/^expect/i.test(action) || /^load\s/i.test(action) || /^save\s/i.test(action) || /^list$/i.test(action)) continue;
    const keyText = action.replace(/^(keys|press)\s+/i, "");
    let tokens;
    try { tokens = parseKeys(keyText); } catch (e) { continue; }
    ({ pc, mode } = simulateModeAndPc(tokens, pc, mode, labelMap));
  }
  return labelMap;
}

function applyLabelMap(tokens, labelMap) {
  if (!tokens.some((t) => t && typeof t === "object" && ("label" in t || "ref" in t))) {
    return tokens;
  }
  return tokens.flatMap((token) => {
    if (token === null || token === undefined) return [token];
    if (typeof token === "object" && "label" in token) return [];
    if (typeof token === "object" && "ref" in token) {
      const addr = labelMap[token.ref];
      if (addr === undefined) throw new Error(`unknown label: &${token.ref}`);
      if (addr < 0 || addr > 72) throw new Error(`label address ${addr} out of range 0-72: &${token.ref}`);
      return [String(Math.floor(addr / 10)), String(addr % 10)];
    }
    return [token];
  });
}

function isSwitchToken(token) {
  const normalized = String(token).toUpperCase().replace(/^:/, "/");
  return ["/RD", "/DG", "/LOAD", "/CLR", "/RUN", "/ON", "/OFF"].includes(normalized);
}

function isRunStopToken(token) {
  return typeof token === "string" && String(token).toUpperCase() === "R/S";
}

// ── SantronRunner class ───────────────────────────────────────────────────────

function traceProgramStep({ steps: stepNo, pc, code, name, display }) {
  console.log(`${String(stepNo).padStart(4, " ")}  ${String(pc).padStart(2, "0")}  ${String(code).padStart(3, "0")}  ${(name || codeName(code) || "?").padEnd(5)}  ${display.trim()}`);
}

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

function normalizeScenarioText(text) {
  return text
    .split(/\r?\n/)
    .map((line) => line.replace(/\s*#.*$/, ""))
    .join("\n");
}

function parseScenarioFileArgument(text) {
  const match = text.match(/^(?:"([^"]+)"|'([^']+)'|(\S+))\s*$/);
  if (!match) return null;
  return match[1] || match[2] || match[3];
}

function formatKeyToken(token) {
  if (typeof token === "object" && token !== null) {
    if ("label" in token) return `%${token.label}`;
    if ("ref" in token) return `&${token.ref}`;
    if ("code" in token) return `$${String(token.code).padStart(3, "0")}`;
  }
  return String(token);
}

class SantronRunner {
  constructor() {
    this.calculator = createCalculator();
    this.state = this.calculator.state;
    this._debugRowsPrinted = 0;
  }

  loadProgram(filename) {
    const text = fs.readFileSync(filename, "utf8");
    return this.calculator.loadProgramListing(text);
  }

  saveProgram(filename) {
    fs.writeFileSync(filename, `${this.calculator.makeProgramListing()}\n`, "utf8");
  }

  printProgramList() {
    const { state } = this;
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

  applySwitch(token, options = {}) {
    const { state, calculator } = this;
    const normalized = String(token).toUpperCase().replace(/^:/, "/");
    if (normalized === "/RD") { state.angle = "rad"; return true; }
    if (normalized === "/DG") { state.angle = "deg"; return true; }
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
    if (normalized === "/RUN") { state.mode = "run"; return true; }
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

  executeKeyToken(token) {
    const { calculator } = this;
    if (typeof token === "object" && token !== null && "label" in token) return; // label definition is skipped during execution
    if (typeof token === "object" && token !== null && "ref" in token) {
      throw new Error(`unresolved label reference: &${token.ref} -- applyLabelMap not called?`);
    }
    if (typeof token === "string" && isSwitchToken(token)) {
      if (!this.applySwitch(token)) throw new Error(`unknown switch: ${token}`);
      return;
    }
    if (typeof token === "object" && token !== null && "code" in token) {
      if (token.code === 99) return;
      if (!isValidProgramCode(token.code)) throw new Error(`unknown keycode: ${token.code}`);
      const key = keyFromCode(token.code);
      if (!key) return;
      calculator.execute(key);
      return;
    }
    calculator.execute(token);
  }

  debugProgramStep({ steps: stepNo, pc, code, name, before }) {
    if (this._debugRowsPrinted % 10 === 0) console.log(`\n${debugHeader()}`);
    this._debugRowsPrinted += 1;
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

  programStepObserver(options) {
    if (!options.trace && !options.debug) return null;
    return (step) => {
      const adjustedStep = { ...step, steps: (options.stepOffset || 0) + step.steps };
      if (options.debug) this.debugProgramStep(adjustedStep);
      if (options.trace) traceProgramStep(adjustedStep);
    };
  }

  runUntilStop(options) {
    return this.calculator.runProgram(options.steps, this.programStepObserver(options));
  }

  printRunStopDisplay(options) {
    options.runStopCount = (options.runStopCount || 0) + 1;
    console.log(`R/S ${String(options.runStopCount).padStart(4, "0")}: ${this.calculator.displayText().trim()}`);
  }

  traceKeyToken(token, options) {
    if (!options.debug && !options.keyTrace) return;
    const display = this.calculator.displayText().trim();
    const modeAngle = angleModeText(this.state.angle, this.state.mode);
    const pc = String(this.state.pc).padStart(2, "0");
    console.log(`KEY  ${formatKeyToken(token).padEnd(10)}  ${modeAngle.padEnd(5)}  pc=${pc}  display=${display}`);
  }

  executeKeyTokenAndMaybeRun(token, options, stepOffset = 0) {
    this.traceKeyToken(token, options);
    const { state } = this;
    const wasRunning = state.running;
    const wasRunMode = state.mode === "run";
    const isRunStop = isRunStopToken(token);
    this.executeKeyToken(token);
    if (!wasRunning && state.running) {
      options.stepOffset = stepOffset;
      let steps;
      try {
        steps = this.runUntilStop(options);
      } finally {
        options.stepOffset = 0;
      }
      if (state.running) throw new Error(`program did not stop after ${options.steps} steps`);
      if (isRunStop && wasRunMode) this.printRunStopDisplay(options);
      return steps;
    }
    if (isRunStop && wasRunMode) this.printRunStopDisplay(options);
    return 0;
  }

  statusLine(steps) {
    const { state, calculator } = this;
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

  scenarioExpectationError(expected, action, actionIndex, steps) {
    const actual = this.calculator.displayText().trim();
    const lines = [
      `expect failed at scenario step ${actionIndex + 1}`,
      `action: ${action}`,
      `expected display: ${expected}`,
      `actual display:   ${actual}`,
      "",
      debugHeader(),
      this.statusLine(steps),
      formatDebugMemories(this.state.memories),
    ];
    if (steps > 0 && this.state.program.some((code) => code !== 99)) {
      lines.push("", "program:", this.calculator.makeProgramListing());
    }
    return lines.join("\n");
  }

  runScenario(text, options) {
    let steps = 0;
    const actions = normalizeScenarioText(text).split(/[;\r\n]+/).map((part) => part.trim()).filter(Boolean);
    const labelMap = buildLabelMap(actions, this.state.pc, this.state.mode);
    actions.forEach((action, actionIndex) => {
      const expectMatch = action.match(/^expect(?:-display)?\s+(.+)$/i);
      if (expectMatch) {
        const expected = expectMatch[1].trim();
        const actual = this.calculator.displayText().trim();
        if (actual !== expected) {
          throw new Error(this.scenarioExpectationError(expected, action, actionIndex, steps));
        }
        return;
      }

      const loadMatch = action.match(/^load\s+(.+)$/i);
      if (loadMatch) {
        const filename = parseScenarioFileArgument(loadMatch[1].trim());
        if (!filename) throw new Error(`invalid load command: ${action}`);
        this.loadProgram(filename);
        return;
      }

      const saveMatch = action.match(/^save\s+(.+)$/i);
      if (saveMatch) {
        const filename = parseScenarioFileArgument(saveMatch[1].trim());
        if (!filename) throw new Error(`invalid save command: ${action}`);
        this.saveProgram(filename);
        return;
      }

      if (/^list$/i.test(action)) {
        this.printProgramList();
        return;
      }

      const keyText = action.replace(/^(keys|press)\s+/i, "");
      const tokens = applyLabelMap(parseKeys(keyText), labelMap);
      tokens.forEach((token) => {
        steps += this.executeKeyTokenAndMaybeRun(token, options, steps);
      });
    });
    return steps;
  }

  printSummary(steps) {
    console.log(`\n${debugHeader()}`);
    console.log(this.statusLine(steps));
    console.log(formatDebugMemories(this.state.memories));
  }

  run(options) {
    this.calculator.resetCalculatorState();
    if (options.program) this.loadProgram(options.program);
    if (options.mode) options.mode.forEach((token) => {
      if (!this.applySwitch(token, { initial: true })) throw new Error(`unknown switch: ${token}`);
    });
    if (Number.isFinite(options.pc)) this.state.pc = options.pc;
    if (Number.isFinite(options.dp)) {
      this.state.pendingPrecision = true;
      this.calculator.execute(String(options.dp));
    }

    for (let i = 0; i < options.sst; i += 1) this.calculator.runSingleProgramStep();

    let steps = null;
    try {
      if (options.scenarioFile) {
        const scenarioText = fs.readFileSync(options.scenarioFile, "utf8");
        steps = this.runScenario(scenarioText, options);
      }
      if (options.scenario) steps = this.runScenario(options.scenario, options);
    } catch (error) {
      console.error(error.message);
      process.exit(1);
    }

    this.printSummary(steps);
    if (options.mem) this.state.memories.forEach((value, idx) => console.log(`M${idx}: ${value}`));
  }
}

if (require.main === module) {
  let options;
  try {
    options = parseArgs(process.argv.slice(2));
  } catch (error) {
    console.error(error.message);
    usage(2);
  }
  new SantronRunner().run(options);
}

module.exports = { SantronRunner };

