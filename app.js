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

const KEY_CODES = {
  "+": 10, "-": 11, "*": 12, "/": 13, "Y^X": 14, "1/X": 17, "X2": 18, "SQRT": 19,
  "SIN": 20, "COS": 21, "TAN": 22, "ASIN": 30, "ACOS": 31, "ATAN": 32,
  "LN": 40, "LOG": 41, "EX": 42, "10X": 43, "M+": 50, "M-": 51, "MX": 52, "MDIV": 53,
  "STO": 55, "RCL": 56, "C/CE": 60, "+/-": 62, "EXP": 63, "X<>Y": 64, "PI": 65,
  ".": 70, "PS": 71, "=": 80, "(": 81, ")": 82, "R/S": 90, "GTO": 93, "SKP": 96,
};

for (let i = 0; i <= 9; i += 1) KEY_CODES[String(i)] = 100 + i;

const PI_VALUE = 3.141592654;

const KEYS = [
  [{ key: "F", label: "F", color: "red" }, { key: "SIN", label: "sin", shift: "sin-1", color: "gray" }, { key: "COS", label: "cos", shift: "cos-1", color: "gray" }, { key: "TAN", label: "tan", shift: "tan-1", color: "gray" }, { key: "PS", label: "PS", color: "gray" }],
  [{ key: "EX", label: "e^x", shift: "ln", color: "gray" }, { key: "LOG", label: "log", shift: "10^x", color: "gray" }, { key: "Y^X", label: "y^x", color: "gray" }, { key: "STO", label: "STOn", color: "gray" }, { key: "RCL", label: "RCLn", color: "gray" }],
  [{ key: "SQRT", label: "sqrt", shift: "x^2", color: "red" }, { key: "1/X", label: "1/x", color: "red" }, { key: "X<>Y", label: "x-y", color: "blue" }, { key: "(", label: "(", color: "blue" }, { key: ")", label: ")", color: "blue" }],
  [{ key: "EXP", label: "EXP", color: "red" }, { key: "7", label: "7", color: "white" }, { key: "8", label: "8", color: "white" }, { key: "9", label: "9", color: "white" }, { key: "/", label: "÷", shift: "Mn÷", color: "blue" }],
  [{ key: "PI", label: "π", color: "red" }, { key: "4", label: "4", color: "white" }, { key: "5", label: "5", color: "white" }, { key: "6", label: "6", color: "white" }, { key: "*", label: "×", shift: "Mn×", color: "blue" }],
  [{ key: "+/-", label: "+/-", color: "red" }, { key: "1", label: "1", color: "white" }, { key: "2", label: "2", color: "white" }, { key: "3", label: "3", color: "white" }, { key: "+", label: "+", shift: "Mn+", color: "blue" }],
  [{ key: "C/CE", label: "C/CE", color: "red" }, { key: "0", label: "0", color: "white" }, { key: ".", label: ".", color: "white" }, { key: "-", label: "-", shift: "Mn-", color: "blue" }, { key: "=", label: "=", color: "blue" }],
];

const state = {
  x: "0",
  y: 0,
  pending: null,
  expression: null,
  parenStack: [],
  entering: false,
  exponentEntry: null,
  shift: false,
  pc: 0,
  program: Array(72).fill(99),
  memories: Array(10).fill(0),
  mode: "run",
  angle: "deg",
  displayFormat: { mode: "auto", decimals: null },
  runSpeed: 0,
  runDisplayMode: "blank",
  heldRunDisplay: null,
  power: "on",
  running: false,
  programPaused: false,
  pendingPrecision: false,
  pendingGotoDigits: null,
  runTimer: null,
};

const display = document.querySelector("#display");
const keypad = document.querySelector("#keypad");
const programList = document.querySelector("#programList");
const programFile = document.querySelector("#programFile");
const programName = document.querySelector("#programName");
const serverProgramSelect = document.querySelector("#serverProgramSelect");
const runSpeed = document.querySelector("#runSpeed");
const runSpeedValue = document.querySelector("#runSpeedValue");

let renderQueued = false;
let runDisplayBlanked = false;
let runDisplayHeld = false;
let serverProgramManifest = null;
let programRenderDirty = true;
let renderedProgramCursor = null;
let renderedProgramCursorVisible = null;
let lastPointerKeyTime = 0;
let keyPointerStart = null;

function makeDisplay() {
  display.innerHTML = "";
  for (let i = 0; i < 12; i += 1) {
    const digit = document.createElement("div");
    digit.className = "digit";
    const block = document.createElement("div");
    block.className = "segment-block";
    "abcdefg".split("").forEach((name) => {
      const wrap = document.createElement("div");
      wrap.className = `seg-glow ${name}`;
      const seg = document.createElement("span");
      seg.className = `seg ${name}`;
      wrap.append(seg);
      block.append(wrap);
    });
    const dot = document.createElement("span");
    dot.className = "dot";
    digit.append(block, dot);
    display.append(digit);
  }
}

function makeKeypad() {
  keypad.innerHTML = "";
  KEYS.flat().forEach((item) => {
    const wrap = document.createElement("div");
    wrap.className = "key-wrap";
    const shift = document.createElement("div");
    shift.className = "shift";
    if (item.shift) {
      shift.innerHTML = item.shift.replace(/[×÷]/g, (symbol) => `<span class="operator-symbol">${symbol}</span>`);
    }
    const button = document.createElement("button");
    button.className = `pill ${item.color}`;
    if (item.label.length >= 4) button.classList.add("long-label");
    button.type = "button";
    button.dataset.key = item.key;
    button.textContent = item.label;
    wrap.append(shift, button);
    keypad.append(wrap);
  });
}

function displayText() {
  if (state.power === "off") return " ".repeat(12);
  if (state.running && state.runDisplayMode === "blank") return " ".repeat(12);
  if (state.running && state.runDisplayMode === "hold" && state.heldRunDisplay !== null) return state.heldRunDisplay;
  return normalDisplayText();
}

function normalDisplayText() {
  if (state.mode === "load") {
    const code = String(state.program[state.pc]).padStart(3, "0");
    return `${code} ${String(state.pc).padStart(2, "0")}`.padStart(12, " ");
  }
  const n = Number(state.x);
  if (!Number.isFinite(n)) return "Error".padStart(12, " ");
  if (state.exponentEntry) return fitDisplayText(formatExponential(n));
  if (state.entering) return fitDisplayText(withVisibleDecimalPoint(state.x));
  if (Object.is(n, -0) || n === 0) {
    const zero = state.displayFormat.mode === "fixed" ? Number(0).toFixed(state.displayFormat.decimals) : "0";
    return fitDisplayText(withVisibleDecimalPoint(zero));
  }
  let s;
  if (state.displayFormat.mode === "fixed") {
    s = n.toFixed(state.displayFormat.decimals);
    if (displayCellCount(s) > 12) s = formatExponential(n);
  } else {
    s = String(Number(n.toPrecision(8)));
    if (s.includes("e")) s = formatExponential(n);
    if (displayCellCount(s) > 12) s = formatExponential(n);
    if (digitCount(s) > 8) s = formatExponential(n);
  }
  return fitDisplayText(withVisibleDecimalPoint(s));
}

function formatExponential(value) {
  const [mantissa, exponent] = value.toExponential(7).split("e");
  const exponentNumber = Number(exponent);
  const mantissaSign = value < 0 ? "-" : " ";
  const exponentSign = exponentNumber < 0 ? "-" : " ";
  const mantissaValue = mantissa
    .replace("-", "")
    .replace(/(\.\d*?)0+$/, "$1")
    .replace(/\.$/, "");
  const mantissaDigits = mantissaValue.match(/\d/g) || [];
  const mantissaText = mantissaDigits.length > 8
    ? mantissaValue.replace(/\d/g, () => mantissaDigits.shift() ?? "").replace(/\D+$/, "")
    : mantissaValue;
  const mantissaPadding = " ".repeat(Math.max(0, 8 - digitCount(mantissaText)));
  const exponentDigits = String(Math.abs(exponentNumber)).padStart(2, "0").slice(-2);
  return `${mantissaSign}${mantissaPadding}${mantissaText}${exponentSign}${exponentDigits}`;
}

function withVisibleDecimalPoint(text) {
  if (text.includes(".") || /[A-Za-z]/.test(text)) return text;
  return `${text}.`;
}

function digitCount(text) {
  return (text.match(/\d/g) || []).length;
}

function displayCellCount(text) {
  return [...text].filter((ch) => ch !== ".").length;
}

function fitDisplayText(text) {
  const chars = [];
  let cells = 0;
  for (const ch of text) {
    if (ch !== ".") {
      if (cells >= 12) break;
      cells += 1;
    }
    chars.push(ch);
  }
  return `${" ".repeat(Math.max(0, 12 - cells))}${chars.join("")}`;
}

function renderDisplay(text) {
  const chars = [];
  const dots = [];
  for (const ch of text) {
    if (ch === ".") {
      if (dots.length) dots[dots.length - 1] = true;
    } else {
      chars.push(ch);
      dots.push(false);
    }
  }
  while (chars.length < 12) {
    chars.unshift(" ");
    dots.unshift(false);
  }
  chars.slice(-12).forEach((ch, idx) => {
    const digit = display.children[idx];
    const lit = SEGMENTS[ch] || "";
    digit.querySelector(".segment-block").classList.toggle("lit", lit.length > 0);
    digit.querySelectorAll(".seg").forEach((seg) => {
      const on = lit.includes(seg.classList[1]);
      seg.classList.toggle("on", on);
      seg.parentElement.classList.toggle("on", on);
    });
    digit.querySelector(".dot").classList.toggle("on", dots.slice(-12)[idx]);
  });
}

function renderProgram(showActive = true) {
  if (!programRenderDirty && renderedProgramCursor === state.pc && renderedProgramCursorVisible === showActive) return;

  programList.innerHTML = "";
  const fragment = document.createDocumentFragment();
  state.program.forEach((code, idx) => {
    const li = document.createElement("li");
    li.className = showActive && idx === state.pc ? "active" : "";
    li.textContent = `${String(idx).padStart(2, "0")}  ${String(code).padStart(3, "0")}  ${codeName(code)}`;
    fragment.append(li);
  });
  programList.append(fragment);
  programRenderDirty = false;
  renderedProgramCursor = state.pc;
  renderedProgramCursorVisible = showActive;
}

function markProgramRenderDirty() {
  programRenderDirty = true;
}

function enterManualGoto() {
  state.pendingGotoDigits = [];
  state.entering = false;
}

function applyManualGotoDigit(key) {
  if (!state.pendingGotoDigits) return false;
  if (!/^[0-9]$/.test(key)) {
    state.pendingGotoDigits = null;
    return false;
  }

  state.pendingGotoDigits.push(Number(key));
  if (state.pendingGotoDigits.length === 2) {
    const target = state.pendingGotoDigits[0] * 10 + state.pendingGotoDigits[1];
    state.pc = target % state.program.length;
    state.pendingGotoDigits = null;
  }
  render();
  return true;
}

function applyPrecisionDigit(key) {
  if (!state.pendingPrecision) return false;
  if (!/^[0-9]$/.test(key)) {
    state.pendingPrecision = false;
    return false;
  }

  const digit = Number(key);
  if (digit <= 7) {
    state.displayFormat = { mode: "fixed", decimals: digit };
  } else {
    state.displayFormat = { mode: "auto", decimals: null };
  }
  state.pendingPrecision = false;
  state.entering = false;
  return true;
}

function codeName(code) {
  const found = Object.entries(KEY_CODES).find(([, value]) => value === code);
  return found ? found[0] : code === 99 ? "END" : "";
}

function programListingLine(code, idx) {
  const address = String(idx).padStart(2, "0");
  const keyCode = String(code).padStart(3, "0");
  const name = codeName(code);
  return `${address} ${keyCode} ${name}`.trimEnd();
}

function makeProgramListing() {
  return state.program.map((code, idx) => programListingLine(code, idx)).join("\n");
}

function isValidProgramCode(code) {
  return code === 99 || Object.values(KEY_CODES).includes(code);
}

function parseProgramListing(text) {
  const program = Array(72).fill(99);
  const seen = new Set();

  text.split(/\r?\n/).forEach((line, lineIdx) => {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith("#") || trimmed.startsWith(";")) return;

    const match = trimmed.match(/^(\d{1,2})\s+(\d{2,3})\b/);
    if (!match) {
      throw new Error(`Zeile ${lineIdx + 1}: erwartet "Zelle KeyCode Zeichen"`);
    }

    const address = Number(match[1]);
    const code = Number(match[2]);
    if (address < 0 || address >= state.program.length) {
      throw new Error(`Zeile ${lineIdx + 1}: Adresse ${match[1]} liegt nicht zwischen 00 und 71`);
    }
    if (!isValidProgramCode(code)) {
      throw new Error(`Zeile ${lineIdx + 1}: Keycode ${match[2]} ist unbekannt`);
    }

    program[address] = code;
    seen.add(address);
  });

  if (!seen.size) throw new Error("Die Datei enthaelt kein Programmlisting");
  return program;
}

function programFilename() {
  const fallback = "santron-626-program";
  const rawName = (programName?.value || fallback).trim() || fallback;
  const safeName = rawName.replace(/[\\/:*?"<>|]+/g, "-").replace(/\s+/g, " ").trim() || fallback;
  return safeName.toLowerCase().endsWith(".lst") ? safeName : `${safeName}.lst`;
}

function programNameFromFile(filename) {
  return filename.replace(/\.[^.]+$/, "") || "santron-626-program";
}

function runSpeedFromSlider(value) {
  const step = Number(value);
  if (step === 0) return 0;
  if (step <= 20) return step;
  if (step <= 28) return 20 + (step - 20) * 10;
  if (step <= 46) return 100 + (step - 28) * 50;
  return 1000 + (step - 46) * 200;
}

async function saveProgramFile() {
  const listing = `${makeProgramListing()}\n`;
  if ("showSaveFilePicker" in window) {
    try {
      const handle = await window.showSaveFilePicker({
        suggestedName: programFilename(),
        types: [{
          description: "Santron 626 Programmlisting",
          accept: { "text/plain": [".lst", ".txt", ".626"] },
        }],
      });
      const writable = await handle.createWritable();
      await writable.write(listing);
      await writable.close();
      return;
    } catch (error) {
      if (error.name === "AbortError") return;
      alert("Die Programmdatei konnte nicht gespeichert werden.");
      return;
    }
  }

  const blob = new Blob([listing], { type: "text/plain" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = programFilename();
  link.click();
  URL.revokeObjectURL(link.href);
}

function loadProgramFile(file) {
  if (!file) return;
  const reader = new FileReader();
  reader.addEventListener("load", () => {
    try {
      applyProgramListing(String(reader.result || ""), file.name);
    } catch (error) {
      alert(error.message);
    }
  });
  reader.addEventListener("error", () => {
    alert("Die Programmdatei konnte nicht gelesen werden.");
  });
  reader.readAsText(file);
}

function applyProgramListing(text, name) {
  state.program = parseProgramListing(text);
  state.pc = 0;
  state.shift = false;
  if (programName) programName.value = programNameFromFile(name);
  markProgramRenderDirty();
  render();
}

async function loadServerProgramManifest() {
  if (serverProgramManifest) return serverProgramManifest;
  const response = await fetch("programs/manifest.json", { cache: "no-store" });
  if (!response.ok) throw new Error("Programmliste konnte nicht geladen werden.");
  const data = await response.json();
  const programs = Array.isArray(data.programs) ? data.programs : [];
  serverProgramManifest = programs.filter((item) => item && item.name && item.file);
  return serverProgramManifest;
}

function fillServerProgramSelect(programs) {
  serverProgramSelect.innerHTML = "";
  const placeholder = document.createElement("option");
  placeholder.value = "";
  placeholder.textContent = "Programm aus Repository öffnen...";
  serverProgramSelect.append(placeholder);

  programs.forEach((program, idx) => {
    const option = document.createElement("option");
    option.value = String(idx);
    option.textContent = program.name;
    serverProgramSelect.append(option);
  });
}

async function initializeServerPrograms() {
  try {
    const programs = await loadServerProgramManifest();
    fillServerProgramSelect(programs);
    serverProgramSelect.disabled = programs.length === 0;
  } catch (error) {
    serverProgramSelect.innerHTML = "";
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "Repository-Programme nicht verfügbar";
    serverProgramSelect.append(option);
    serverProgramSelect.disabled = true;
  }
}

async function openSelectedServerProgram() {
  if (serverProgramSelect.value === "") return;
  const programs = await loadServerProgramManifest();
  const program = programs[Number(serverProgramSelect.value)];
  if (!program) return;

  try {
    const response = await fetch(`programs/${program.file}`, { cache: "no-store" });
    if (!response.ok) throw new Error(`${program.name} konnte nicht geladen werden.`);
    applyProgramListing(await response.text(), program.file);
    serverProgramSelect.value = "";
  } catch (error) {
    alert(error.message);
  }
}

function resetCalculatorState() {
  stopProgram();
  state.x = "0";
  state.y = 0;
  state.pc = 0;
  state.program.fill(99);
  state.memories.fill(0);
  state.shift = false;
  state.entering = false;
  state.exponentEntry = null;
  state.programPaused = false;
  state.displayFormat = { mode: "auto", decimals: null };
  state.pendingPrecision = false;
  state.pendingGotoDigits = null;
  state.pendingMemory = null;
  state.parenStack = [];
  resetExpression();
  markProgramRenderDirty();
}

function renderNow() {
  if (state.running && state.runDisplayMode === "blank") {
    if (!runDisplayBlanked) {
      renderDisplay(" ".repeat(12));
      runDisplayBlanked = true;
    }
    runDisplayHeld = false;
  } else if (state.running && state.runDisplayMode === "hold") {
    if (!runDisplayHeld) {
      renderDisplay(displayText());
      runDisplayHeld = true;
    }
    runDisplayBlanked = false;
  } else {
    renderDisplay(displayText());
    runDisplayBlanked = false;
    runDisplayHeld = false;
  }
  if (state.running && state.runSpeed < 20) {
    if (programList.dataset.cursorHidden !== "true") {
      renderProgram(false);
      programList.dataset.cursorHidden = "true";
    }
  } else {
    renderProgram();
    programList.dataset.cursorHidden = "false";
  }
}

function render() {
  if (renderQueued) return;
  renderQueued = true;
  requestAnimationFrame(() => {
    renderQueued = false;
    renderNow();
  });
}

function renderPower() {
  const off = state.power === "off";
  document.body.classList.toggle("power-off", off);
  display.closest(".display-wrap").classList.toggle("power-off", off);
}

function roundInternal(value) {
  return Number.isFinite(value) ? Number(value.toPrecision(10)) : value;
}

function setX(value) {
  const rounded = roundInternal(value);
  state.x = String(Number.isFinite(rounded) ? rounded : "Error");
  state.entering = false;
  state.exponentEntry = null;
}

function updateExponentValue() {
  const entry = state.exponentEntry;
  const exponent = entry.sign * Number(entry.digits || "0");
  const value = roundInternal(entry.mantissa * Math.pow(10, exponent));
  state.x = String(Number.isFinite(value) ? value : "Error");
  state.entering = false;
}

function startExponentEntry() {
  state.exponentEntry = { mantissa: Number(state.x), sign: 1, digits: "" };
  state.entering = false;
}

function pushExponentDigit(key) {
  if (!state.exponentEntry || !/^[0-9]$/.test(key)) return false;
  if (state.exponentEntry.digits.length >= 2) return true;
  state.exponentEntry.digits += key;
  updateExponentValue();
  return true;
}

function toggleExponentSign() {
  if (!state.exponentEntry) return false;
  state.exponentEntry.sign *= -1;
  updateExponentValue();
  return true;
}

function finishExponentEntry() {
  state.exponentEntry = null;
}

function pushDigit(key) {
  if (state.exponentEntry && key === ".") return;
  if (pushExponentDigit(key)) return;
  if (!state.entering || state.x === "0") {
    state.x = key === "." ? "0." : key;
    state.entering = true;
    return;
  }
  if (key === "." && state.x.includes(".")) return;
  if (state.x.replace("-", "").replace(".", "").length < 10) state.x += key;
}

function newExpression() {
  return { total: 0, addOp: "+", term: null, mulOp: null };
}

function expression() {
  if (!state.expression) state.expression = newExpression();
  return state.expression;
}

function applyAdd(total, op, value) {
  return roundInternal(op === "-" ? total - value : total + value);
}

function applyMul(left, op, right) {
  if (left === null) return roundInternal(right);
  if (op === "*") return roundInternal(left * right);
  if (op === "/") return roundInternal(left / right);
  if (op === "Y^X") return roundInternal(Math.pow(left, right));
  return roundInternal(right);
}

function commitTerm(value) {
  const expr = expression();
  expr.term = applyMul(expr.term, expr.mulOp, value);
  expr.mulOp = null;
  state.y = expr.term;
  return expr.term;
}

function commitExpression(value) {
  const expr = expression();
  const term = commitTerm(value);
  expr.total = applyAdd(expr.total, expr.addOp, term);
  expr.term = null;
  expr.addOp = "+";
  state.y = expr.total;
  return expr.total;
}

function queueOperator(key) {
  finishExponentEntry();
  const value = Number(state.x);
  if (["*", "/", "Y^X"].includes(key)) {
    const term = commitTerm(value);
    setX(term);
    expression().mulOp = key;
  } else {
    const total = commitExpression(value);
    setX(total);
    expression().addOp = key;
  }
  state.pending = key;
  state.entering = false;
}

function currentExpressionValue() {
  const expr = expression();
  const value = Number(state.x);
  const term = applyMul(expr.term, expr.mulOp, value);
  return applyAdd(expr.total, expr.addOp, term);
}

function resetExpression() {
  state.expression = newExpression();
  state.pending = null;
}

function openParen() {
  state.parenStack.push(expression());
  state.expression = newExpression();
  state.entering = false;
}

function closeParen() {
  if (!state.parenStack.length) return;
  const value = currentExpressionValue();
  state.expression = state.parenStack.pop();
  setX(value);
}

function unary(key) {
  const x = Number(state.x);
  const radians = state.angle === "rad" ? x : x * Math.PI / 180;
  const fromRadians = (value) => state.angle === "rad" ? value : value * 180 / Math.PI;
  const map = {
    "SIN": Math.sin(radians),
    "COS": Math.cos(radians),
    "TAN": Math.tan(radians),
    "ASIN": fromRadians(Math.asin(x)),
    "ACOS": fromRadians(Math.acos(x)),
    "ATAN": fromRadians(Math.atan(x)),
    "LN": Math.log(x),
    "LOG": Math.log10(x),
    "EX": Math.exp(x),
    "10X": Math.pow(10, x),
    "SQRT": Math.sqrt(x),
    "X2": x * x,
    "1/X": 1 / x,
  };
  if (key in map) setX(map[key]);
}

function execute(key, fromProgram = false) {
  if (state.power === "off") return;
  if (!fromProgram && state.running && key !== "R/S") return;
  if (applyPrecisionDigit(key)) {
    if (!fromProgram) render();
    return;
  }
  if (!fromProgram && applyManualGotoDigit(key)) return;
  if (state.mode === "clear" && !fromProgram) {
    if (key === "R/S") {
      state.program.fill(99);
      state.pc = 0;
      markProgramRenderDirty();
    } else if (key === "BST" || key === "SST") {
      state.program[state.pc] = 99;
      state.pc = key === "BST"
        ? (state.pc + state.program.length - 1) % state.program.length
        : (state.pc + 1) % state.program.length;
      markProgramRenderDirty();
    }
    render();
    return;
  }
  if (state.mode === "load" && !fromProgram && key !== "BST" && key !== "SST") {
    if (key === "F") {
      state.shift = !state.shift;
      render();
      return;
    }
    state.program[state.pc] = KEY_CODES[key] ?? 99;
    state.pc = (state.pc + 1) % state.program.length;
    markProgramRenderDirty();
    render();
    return;
  }
  if (state.pendingMemory && /^[0-9]$/.test(key)) {
    const idx = Number(key);
    const x = roundInternal(Number(state.x));
    if (state.pendingMemory === "STO") state.memories[idx] = x;
    if (state.pendingMemory === "RCL") setX(state.memories[idx]);
    if (state.pendingMemory === "M+") state.memories[idx] = roundInternal(state.memories[idx] + x);
    if (state.pendingMemory === "M-") state.memories[idx] = roundInternal(state.memories[idx] - x);
    if (state.pendingMemory === "MX") state.memories[idx] = roundInternal(state.memories[idx] * x);
    if (state.pendingMemory === "MDIV") state.memories[idx] = roundInternal(state.memories[idx] / x);
    state.pendingMemory = null;
    state.exponentEntry = null;
    state.entering = false;
  } else if (/^[0-9.]$/.test(key)) pushDigit(key);
  else if (["+", "-", "*", "/", "Y^X"].includes(key)) {
    queueOperator(key);
  } else if (key === "=") {
    setX(currentExpressionValue());
    resetExpression();
    state.parenStack = [];
  } else if (key === "C/CE") {
    state.x = "0"; state.y = 0; resetExpression(); state.parenStack = []; state.exponentEntry = null; state.entering = false;
  } else if (key === "+/-") {
    if (toggleExponentSign()) {
      // Exponent sign changed.
    } else {
      setX(-Number(state.x));
    }
  } else if (key === "EXP") {
    startExponentEntry();
  } else if (key === "PI") {
    setX(PI_VALUE);
  } else if (key === "X<>Y") {
    const old = Number(state.x);
    setX(state.y);
    state.y = old;
  } else if (["SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN", "LN", "LOG", "EX", "10X", "SQRT", "X2", "1/X"].includes(key)) {
    unary(key);
  } else if (key === "F") {
    state.shift = !state.shift;
  } else if (key === "(") {
    openParen();
  } else if (key === ")") {
    closeParen();
  } else if (key === "GTO" && !fromProgram && state.mode === "run") {
    enterManualGoto();
  } else if (key === "PS") {
    state.pendingPrecision = true;
    state.entering = false;
  } else if (["STO", "RCL", "M+", "M-", "MX", "MDIV"].includes(key)) {
    state.pendingMemory = key;
  } else if (key === "BST") {
    state.pc = (state.pc + state.program.length - 1) % state.program.length;
  } else if (key === "SST") {
    state.pc = (state.pc + 1) % state.program.length;
  } else if (key === "R/S") {
    if (fromProgram) {
      pauseProgram();
    } else {
      toggleProgramRun();
    }
  } else if (key === "SKP" && Number(state.x) < 0) {
    state.pc = (state.pc + 1) % state.program.length;
  }
  if (!fromProgram) render();
}

function keyFromCode(code) {
  const found = Object.entries(KEY_CODES).find(([, value]) => value === code);
  return found ? found[0] : null;
}

function toggleProgramRun() {
  if (state.mode !== "run") return;
  if (state.running) {
    stopProgram();
    return;
  }
  startProgram();
}

function startProgram() {
  if (state.running || state.power === "off") return;
  state.heldRunDisplay = normalDisplayText();
  resetExpression();
  state.parenStack = [];
  state.running = true;
  state.programPaused = false;
  scheduleProgramStep(0);
  render();
}

function pauseProgram() {
  state.running = false;
  state.programPaused = true;
  state.heldRunDisplay = null;
  clearProgramTimer();
  render();
}

function stopProgram() {
  state.running = false;
  state.programPaused = false;
  state.heldRunDisplay = null;
  clearProgramTimer();
  render();
}

function clearProgramTimer() {
  if (state.runTimer !== null) {
    clearTimeout(state.runTimer);
    state.runTimer = null;
  }
}

function scheduleProgramStep(delay = state.runSpeed) {
  clearProgramTimer();
  state.runTimer = setTimeout(stepProgram, delay);
}

function runOneProgramStep() {
  if (!state.running || state.mode !== "run" || state.power === "off") {
    stopProgram();
    return false;
  }

  const code = state.program[state.pc];
  if (code === 99) {
    stopProgram();
    return false;
  }

  state.pc = (state.pc + 1) % state.program.length;
  executeProgramCode(code);
  return state.running;
}

function stepProgram() {
  state.runTimer = null;

  if (state.runSpeed === 0) {
    for (let i = 0; i < 1000 && state.running; i += 1) {
      runOneProgramStep();
    }
  } else {
    runOneProgramStep();
  }

  if (state.running) {
    render();
    scheduleProgramStep();
  }
}

function digitFromCode(code) {
  return code >= 100 && code <= 109 ? code - 100 : null;
}

function readGotoAddress() {
  const tens = digitFromCode(state.program[state.pc]);
  const ones = digitFromCode(state.program[(state.pc + 1) % state.program.length]);
  state.pc = (state.pc + 2) % state.program.length;
  if (tens === null || ones === null) return null;
  return tens * 10 + ones;
}

function executeProgramCode(code) {
  const key = keyFromCode(code);
  if (!key) return;

  if (key === "GTO") {
    const target = readGotoAddress();
    if (target !== null) state.pc = target % state.program.length;
    return;
  }

  if (key === "SKP") {
    if (Number(state.x) < 0) {
      state.pc = state.program[state.pc] === KEY_CODES["GTO"]
        ? (state.pc + 3) % state.program.length
        : (state.pc + 1) % state.program.length;
    }
    return;
  }

  execute(key, true);
}

document.addEventListener("click", (event) => {
  if (performance.now() - lastPointerKeyTime < 500) return;
  const button = event.target.closest("[data-key]");
  if (!button) return;
  pressKeyButton(button);
});

document.addEventListener("pointerdown", (event) => {
  const button = event.target.closest("[data-key]");
  keyPointerStart = button && event.pointerType !== "mouse"
    ? { id: event.pointerId, x: event.clientX, y: event.clientY }
    : null;
});

document.addEventListener("pointerup", (event) => {
  const button = event.target.closest("[data-key]");
  if (!button || event.pointerType === "mouse") return;
  if (!keyPointerStart || keyPointerStart.id !== event.pointerId) return;
  const distance = Math.hypot(event.clientX - keyPointerStart.x, event.clientY - keyPointerStart.y);
  keyPointerStart = null;
  if (distance > 10) return;
  event.preventDefault();
  lastPointerKeyTime = performance.now();
  pressKeyButton(button);
});

function pressKeyButton(button) {
  vibrateKeyFeedback();
  let key = button.dataset.key;
  if (state.shift) {
    const shifted = {
      SIN: "ASIN", COS: "ACOS", TAN: "ATAN", EX: "LN", LOG: "10X", SQRT: "X2",
      "+": "M+", "-": "M-", "*": "MX", "/": "MDIV",
    };
    key = shifted[key] || key;
    state.shift = false;
  }
  execute(key);
}

function vibrateKeyFeedback() {
  if (!("vibrate" in navigator)) return;
  navigator.vibrate(8);
}

document.querySelectorAll("input[name='mode']").forEach((input) => {
  input.addEventListener("change", () => {
    if (input.checked) state.mode = input.value;
    if (state.mode !== "run") stopProgram();
    state.pendingGotoDigits = null;
    state.pendingPrecision = false;
    render();
  });
});

document.querySelector("#angleDeg").addEventListener("change", () => { state.angle = "deg"; render(); });
document.querySelector("#angleRad").addEventListener("change", () => { state.angle = "rad"; render(); });
document.querySelector("#powerOn").addEventListener("change", () => {
  state.power = "on";
  resetCalculatorState();
  renderPower();
  render();
});
document.querySelector("#powerOff").addEventListener("change", () => {
  state.power = "off";
  stopProgram();
  renderPower();
  render();
});
runSpeed.addEventListener("input", () => {
  state.runSpeed = runSpeedFromSlider(runSpeed.value);
  runSpeedValue.textContent = `${state.runSpeed} ms`;
  if (state.running) render();
});
document.querySelectorAll("input[name='runDisplayMode']").forEach((input) => {
  input.addEventListener("change", () => {
    if (input.checked) {
      state.runDisplayMode = input.value;
      if (state.running && state.runDisplayMode === "hold") state.heldRunDisplay = normalDisplayText();
      render();
    }
  });
});
document.querySelector("#clearProgram").addEventListener("click", () => {
  state.program.fill(99);
  state.pc = 0;
  state.pendingGotoDigits = null;
  state.pendingPrecision = false;
  markProgramRenderDirty();
  render();
});

document.querySelector("#saveProgram").addEventListener("click", saveProgramFile);
document.querySelector("#loadProgram").addEventListener("click", () => {
  programFile.click();
});
programFile.addEventListener("change", () => {
  loadProgramFile(programFile.files[0]);
  programFile.value = "";
});
serverProgramSelect.addEventListener("change", openSelectedServerProgram);

function initializeControls() {
  document.querySelector("#powerOn").checked = true;
  document.querySelector("#powerOff").checked = false;
  document.querySelector("#angleDeg").checked = true;
  document.querySelector("#angleRad").checked = false;
  document.querySelector("#modeRun").checked = true;
  document.querySelector("#modeLoad").checked = false;
  document.querySelector("#modeClear").checked = false;
  state.power = "on";
  state.angle = "deg";
  state.mode = "run";
  state.displayFormat = { mode: "auto", decimals: null };
  state.runSpeed = runSpeedFromSlider(runSpeed.value);
  state.runDisplayMode = document.querySelector("input[name='runDisplayMode']:checked").value;
  state.heldRunDisplay = null;
  resetExpression();
  state.pendingPrecision = false;
  state.pendingGotoDigits = null;
  runSpeedValue.textContent = `${state.runSpeed} ms`;
}

function showMobileCalculator() {
  const shell = document.querySelector(".shell");
  const centerPanel = document.querySelector(".center-panel");
  if (!shell || !centerPanel) return;
  if (!window.matchMedia("(max-width: 860px)").matches) return;

  requestAnimationFrame(() => {
    shell.scrollTo({ left: centerPanel.offsetLeft, behavior: "auto" });
  });
}

makeDisplay();
makeKeypad();
initializeControls();
initializeServerPrograms();
renderPower();
renderNow();
showMobileCalculator();
window.addEventListener("pageshow", showMobileCalculator);
