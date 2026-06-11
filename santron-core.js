(function initSantronCore(root, factory) {
  if (typeof module === "object" && module.exports) {
    module.exports = factory();
  } else {
    root.SantronCore = factory();
  }
})(typeof globalThis !== "undefined" ? globalThis : this, function createSantronCore() {
  "use strict";

  const KEY_CODES = {
    "+": 10, "-": 11, "*": 12, "/": 13, "Y^X": 14, "1/X": 17, "X^2": 18, "SQRT": 19,
    "SIN": 20, "COS": 21, "TAN": 22, "ASIN": 30, "ACOS": 31, "ATAN": 32,
    "LN": 40, "LOG": 41, "E^X": 42, "10^X": 43, "M+": 50, "M-": 51, "M*": 52, "M/": 53,
    "STO": 55, "RCL": 56, "C/CE": 60, "+/-": 62, "EXP": 63, "X<>Y": 64, "PI": 65,
    ".": 70, "PS": 71, "=": 80, "(": 81, ")": 82, "R/S": 90, "GOTO": 93, "SKP": 96,
  };

  for (let i = 0; i <= 9; i += 1) KEY_CODES[String(i)] = 100 + i;

  const PI_VALUE = 3.141592653;
  const PROGRAM_SIZE = 72;
  const MEMORY_COMMANDS = ["STO", "RCL", "M+", "M-", "M*", "M/"];

  function createState() {
    return {
      x: "0",
      y: 0,
      pending: null,
      parenStack: [],
      entering: false,
      exponentEntry: null,
      shift: false,
      pc: 0,
      program: Array(PROGRAM_SIZE).fill(99),
      memories: Array(10).fill(0),
      mode: "run",
      angle: "deg",
      displayFormat: { mode: "auto", decimals: null },
      programComments: "",
      running: false,
      programPaused: false,
      pendingPrecision: false,
      pendingGotoDigits: null,
      gotoPreview: null,
      power: "on",
    };
  }

  function codeName(code) {
    const found = Object.entries(KEY_CODES).find(([, value]) => value === code);
    return found ? found[0] : "";
  }

  function keyFromCode(code) {
    const found = Object.entries(KEY_CODES).find(([, value]) => value === code);
    return found ? found[0] : null;
  }

  function isValidProgramCode(code) {
    return code === 99 || Object.values(KEY_CODES).includes(code);
  }

  function parseProgramListing(text) {
    const program = Array(PROGRAM_SIZE).fill(99);
    const seen = new Set();
    const comments = [];
    let beforeListing = true;

    text.split(/\r?\n/).forEach((line, lineIdx) => {
      const trimmed = line.trim();
      if (!trimmed || trimmed.startsWith(";")) return;
      if (trimmed.startsWith("#")) {
        if (beforeListing) comments.push(trimmed.replace(/^#\s?/, ""));
        return;
      }
      const match = trimmed.match(/^(\d{1,2})\s+(\d{2,3})\b/);
      if (!match) throw new Error(`Zeile ${lineIdx + 1}: erwartet "Zelle KeyCode Zeichen"`);
      beforeListing = false;
      const address = Number(match[1]);
      const code = Number(match[2]);
      if (address < 0 || address >= PROGRAM_SIZE) {
        throw new Error(`Zeile ${lineIdx + 1}: Adresse ${match[1]} liegt nicht zwischen 00 und 71`);
      }
      if (!isValidProgramCode(code)) throw new Error(`Zeile ${lineIdx + 1}: Keycode ${match[2]} ist unbekannt`);
      program[address] = code;
      seen.add(address);
    });

    if (!seen.size) throw new Error("Die Datei enthaelt kein Programmlisting");
    return { program, comments: comments.join("\n").trim() };
  }

  function programListingLine(code, idx) {
    const address = String(idx).padStart(2, "0");
    const keyCode = String(code).padStart(3, "0");
    const name = codeName(code);
    return `${address} ${keyCode} ${name}`.trimEnd();
  }

  function makeProgramListing(program) {
    return program.map((code, idx) => programListingLine(code, idx)).join("\n");
  }

  function roundInternal(value) {
    if (!Number.isFinite(value) || value === 0) return value;
    const sign = Math.sign(value);
    const abs = Math.abs(value);
    const exponent = Math.floor(Math.log10(abs));
    const scale = 10 ** (9 - exponent);
    const scaled = abs * scale;
    const tolerance = Math.abs(scaled) * Number.EPSILON * 8;
    return sign * (Math.trunc(scaled + tolerance) / scale);
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

  function withVisibleDecimalPoint(text) {
    if (text.includes(".") || /[A-Za-z]/.test(text)) return text;
    return `${text}.`;
  }

  function formatExponential(value) {
    const [mantissa, exponent] = value.toExponential(7).split("e");
    const exponentNumber = Number(exponent);
    const mantissaSign = value < 0 ? "-" : " ";
    const exponentSign = exponentNumber < 0 ? "-" : " ";
    const mantissaValue = mantissa.replace("-", "").replace(/(\.\d*?)0+$/, "$1").replace(/\.$/, "");
    const mantissaDigits = mantissaValue.match(/\d/g) || [];
    const mantissaText = mantissaDigits.length > 8
      ? mantissaValue.replace(/\d/g, () => mantissaDigits.shift() ?? "").replace(/\D+$/, "")
      : mantissaValue;
    const mantissaPadding = " ".repeat(Math.max(0, 8 - digitCount(mantissaText)));
    const mantissaDisplay = mantissaText.includes(".") ? mantissaText : `${mantissaText}.`;
    const exponentDigits = String(Math.abs(exponentNumber)).padStart(2, "0").slice(-2);
    return `${mantissaSign}${mantissaPadding}${mantissaDisplay}${exponentSign}${exponentDigits}`;
  }

  function formatExponentEntry(entry) {
    const mantissaValue = entry.mantissaText.replace("-", "");
    const mantissaText = mantissaValue.includes(".") ? mantissaValue : `${mantissaValue}.`;
    const mantissaCells = displayCellCount(mantissaText);
    const mantissaSign = entry.mantissa < 0 ? "-" : " ";
    const mantissaPadding = " ".repeat(Math.max(0, 8 - mantissaCells));
    const exponentSign = entry.sign < 0 ? "-" : " ";
    const exponentDigits = String(Math.abs(Number(entry.digits || "0"))).padStart(2, "0").slice(-2);
    return `${mantissaSign}${mantissaPadding}${mantissaText}${exponentSign}${exponentDigits}`;
  }

  function createCalculator(initialState) {
    const state = initialState || createState();

    function setX(value) {
      const rounded = roundInternal(value);
      state.x = String(Number.isFinite(rounded) ? rounded : "Error");
      state.entering = false;
      state.exponentEntry = null;
    }

    function formatNumberDisplay(n) {
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

    function displayText() {
      if (state.power === "off") return " ".repeat(12);
      if (state.mode === "load") {
        const code = String(state.program[state.pc]).padStart(3, "0");
        return `${code} ${String(state.pc).padStart(2, "0")}`.padStart(12, " ");
      }
      const n = Number(state.x);
      if (!Number.isFinite(n)) return "Error".padStart(12, " ");
      if (state.exponentEntry) return fitDisplayText(formatExponentEntry(state.exponentEntry));
      if (state.entering) return fitDisplayText(withVisibleDecimalPoint(state.x));
      return formatNumberDisplay(n);
    }

    function applyPrecisionDigit(key) {
      if (!state.pendingPrecision) return false;
      if (!/^[0-9]$/.test(key)) {
        state.pendingPrecision = false;
        return false;
      }
      const digit = Number(key);
      state.displayFormat = digit <= 7 ? { mode: "fixed", decimals: digit } : { mode: "auto", decimals: null };
      state.pendingPrecision = false;
      state.entering = false;
      return true;
    }

    function updateExponentValue() {
      const entry = state.exponentEntry;
      const exponent = entry.sign * Number(entry.digits || "0");
      const value = roundInternal(entry.mantissa * Math.pow(10, exponent));
      state.x = String(Number.isFinite(value) ? value : "Error");
      state.entering = false;
    }

    function startExponentEntry() {
      const mantissaText = state.entering ? state.x : "1";
      const mantissa = Number(mantissaText);
      state.exponentEntry = { mantissa, mantissaText, sign: 1, digits: "" };
      state.x = String(mantissa);
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

    function applyMemoryCommand(command, idx) {
      const x = roundInternal(Number(state.x));
      if (command === "STO") state.memories[idx] = x;
      if (command === "RCL") setX(state.memories[idx]);
      if (command === "M+") state.memories[idx] = roundInternal(state.memories[idx] + x);
      if (command === "M-") state.memories[idx] = roundInternal(state.memories[idx] - x);
      if (command === "M*") state.memories[idx] = roundInternal(state.memories[idx] * x);
      if (command === "M/") state.memories[idx] = roundInternal(state.memories[idx] / x);
      state.exponentEntry = null;
      state.entering = false;
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

    function applyPendingOperator(left, op, right) {
      if (op === "+") return roundInternal(left + right);
      if (op === "-") return roundInternal(left - right);
      if (op === "*") return roundInternal(left * right);
      if (op === "/") return roundInternal(left / right);
      if (op === "Y^X") return roundInternal(Math.pow(left, right));
      return roundInternal(right);
    }

    function commitPendingOperator() {
      if (!state.pending) return Number(state.x);
      const result = applyPendingOperator(state.y, state.pending, Number(state.x));
      state.y = result;
      setX(result);
      state.pending = null;
      return result;
    }

    function queueOperator(key) {
      finishExponentEntry();
      commitPendingOperator();
      state.y = Number(state.x);
      state.pending = key;
      state.entering = false;
    }

    function resetExpression() {
      state.pending = null;
      state.parenStack = [];
    }

    function openParen() {
      state.parenStack.push({ y: state.y, pending: state.pending });
      state.y = 0;
      state.pending = null;
      state.x = "0";
      state.entering = false;
      state.exponentEntry = null;
    }

    function closeParen() {
      if (!state.parenStack.length) return;
      const value = commitPendingOperator();
      const context = state.parenStack.pop();
      state.y = context.y;
      state.pending = context.pending;
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
        "E^X": Math.exp(x),
        "10^X": Math.pow(10, x),
        "SQRT": Math.sqrt(x),
        "X^2": x * x,
        "1/X": 1 / x,
      };
      if (key in map) setX(map[key]);
    }

    function applyManualGotoDigit(key) {
      if (!state.pendingGotoDigits) return false;
      if (!/^[0-9]$/.test(key)) {
        state.pendingGotoDigits = null;
        state.gotoPreview = null;
        return false;
      }
      state.pendingGotoDigits.push(Number(key));
      if (state.pendingGotoDigits.length === 2) {
        const target = state.pendingGotoDigits[0] * 10 + state.pendingGotoDigits[1];
        if (target < state.program.length) state.pc = target;
        state.pendingGotoDigits = null;
      }
      return true;
    }

    function enterManualGoto() {
      state.pendingGotoDigits = [];
      state.entering = false;
    }

    function execute(key, fromProgram = false) {
      if (state.power === "off") return;

      if (state.pendingGotoDigits !== null) {
        if (/^[0-9]$/.test(key)) {
          applyManualGotoDigit(key);
          return;
        }
        state.pendingGotoDigits = null;
        state.gotoPreview = null;
      }

      if (key === "GOTO" && !fromProgram && state.mode === "run") {
        enterManualGoto();
        return;
      }

      if (state.mode === "clear" && !fromProgram) {
        if (key === "R/S") {
          state.program.fill(99);
          state.pc = 0;
        } else if (key === "BST" || key === "SST") {
          state.program[state.pc] = 99;
          state.pc = key === "BST"
            ? (state.pc + state.program.length - 1) % state.program.length
            : (state.pc + 1) % state.program.length;
        }
        return;
      }

      if (state.mode === "load" && !fromProgram && key !== "BST" && key !== "SST") {
        if (key === "F") {
          state.shift = !state.shift;
          return;
        }
        state.program[state.pc] = KEY_CODES[key] ?? 99;
        state.pc = (state.pc + 1) % state.program.length;
        return;
      }

      if (applyPrecisionDigit(key)) return;

      if (state.pendingMemory && /^[0-9]$/.test(key)) {
        applyMemoryCommand(state.pendingMemory, Number(key));
        state.pendingMemory = null;
      } else if (/^[0-9.]$/.test(key)) pushDigit(key);
      else if (["+", "-", "*", "/", "Y^X"].includes(key)) queueOperator(key);
      else if (key === "=") {
        if (state.pending) commitPendingOperator();
        else setX(Number(state.x));
      } else if (key === "C/CE") {
        state.x = "0"; state.y = 0; resetExpression(); state.exponentEntry = null; state.entering = false;
      } else if (key === "+/-") {
        if (!toggleExponentSign()) setX(-Number(state.x));
      } else if (key === "EXP") startExponentEntry();
      else if (key === "PI") setX(PI_VALUE);
      else if (key === "X<>Y") {
        const old = Number(state.x);
        setX(state.y);
        state.y = old;
      } else if (["SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN", "LN", "LOG", "E^X", "10^X", "SQRT", "X^2", "1/X"].includes(key)) unary(key);
      else if (key === "F") state.shift = !state.shift;
      else if (key === "(") openParen();
      else if (key === ")") closeParen();
      else if (key === "GOTO" && !fromProgram && state.mode === "run") enterManualGoto();
      else if (key === "PS") {
        state.pendingPrecision = true;
        state.entering = false;
      } else if (MEMORY_COMMANDS.includes(key)) state.pendingMemory = key;
      else if (key === "BST") {
        if (state.mode !== "run") state.pc = (state.pc + state.program.length - 1) % state.program.length;
      } else if (key === "SST") {
        if (!fromProgram && state.mode === "run") runSingleProgramStep();
        else state.pc = (state.pc + 1) % state.program.length;
      } else if (key === "R/S") {
        if (fromProgram) pauseProgram();
        else toggleProgramRun();
      } else if (key === "SKP") {
        if (!fromProgram && state.mode === "run") skipToNextProgramStop();
        else if (Number(state.x) < 0) state.pc = (state.pc + 1) % state.program.length;
      }
    }

    function startProgram() {
      if (state.running || state.power === "off") return;
      resetExpression();
      state.parenStack = [];
      state.running = true;
      state.programPaused = false;
    }

    function pauseProgram() {
      state.running = false;
      state.programPaused = true;
      state.pendingMemory = null;
    }

    function stopProgram() {
      state.running = false;
      state.programPaused = false;
      state.pendingMemory = null;
    }

    function toggleProgramRun() {
      if (state.running) stopProgram();
      else startProgram();
    }

    function runOneProgramStep() {
      if (!state.running || state.mode !== "run" || state.power === "off" || state.pc >= state.program.length) {
        stopProgram();
        return false;
      }
      const code = state.program[state.pc];
      state.pc += 1;
      if (code === 99) {
        if (state.pc >= state.program.length) stopProgram();
        return true;
      }
      executeProgramCode(code);
      if (state.pc >= state.program.length) stopProgram();
      return state.running;
    }

    function runSingleProgramStep() {
      if (state.mode !== "run" || state.power === "off" || state.running) return;
      state.running = true;
      state.programPaused = false;
      runOneProgramStep();
      state.running = false;
    }

    function skipToNextProgramStop() {
      if (state.mode !== "run" || state.power === "off" || state.running) return;
      while (state.pc < state.program.length) {
        const code = state.program[state.pc];
        state.pc += 1;
        if (code === KEY_CODES["R/S"]) break;
      }
      if (state.pc >= state.program.length) state.pc = state.program.length;
    }

    function digitFromCode(code) {
      return code >= 100 && code <= 109 ? code - 100 : null;
    }

    function readGotoAddress() {
      if (state.pc + 1 >= state.program.length) {
        stopProgram();
        return null;
      }
      const tens = digitFromCode(state.program[state.pc]);
      const ones = digitFromCode(state.program[state.pc + 1]);
      state.pc += 2;
      if (tens === null || ones === null) return null;
      return tens * 10 + ones;
    }

    function readProgramMemoryIndex() {
      if (state.pc >= state.program.length) {
        stopProgram();
        return null;
      }
      const idx = digitFromCode(state.program[state.pc]);
      state.pc += 1;
      if (idx === null) {
        stopProgram();
        return null;
      }
      return idx;
    }

    function executeProgramCode(code) {
      const key = keyFromCode(code);
      if (!key) return;
      if (key === "GOTO") {
        const target = readGotoAddress();
        if (target !== null) {
          if (target >= state.program.length) stopProgram();
          else state.pc = target;
        }
        return;
      }
      if (MEMORY_COMMANDS.includes(key)) {
        const idx = readProgramMemoryIndex();
        if (idx !== null) applyMemoryCommand(key, idx);
        return;
      }
      if (key === "SKP") {
        if (Number(state.x) < 0) {
          const skipWidth = state.program[state.pc] === KEY_CODES["GOTO"] ? 3 : 1;
          state.pc += skipWidth;
          if (state.pc >= state.program.length) stopProgram();
        }
        return;
      }
      execute(key, true);
    }

    function runProgram(maxSteps = 1000, onStep = null) {
      startProgram();
      let steps = 0;
      while (state.running && steps < maxSteps) {
        const pc = state.pc;
        const code = state.program[pc];
        const before = {
          display: displayText(),
          x: state.x,
          y: state.y,
          pending: state.pending,
          memories: state.memories.slice(),
          angle: state.angle,
          mode: state.mode,
          power: state.power,
          running: state.running,
          parenStack: state.parenStack.map((entry) => ({ ...entry })),
        };
        runOneProgramStep();
        steps += 1;
        if (onStep) onStep({ steps, pc, code, name: codeName(code), display: displayText(), before, state });
      }
      return steps;
    }

    function resetCalculatorState() {
      stopProgram();
      const fresh = createState();
      Object.keys(fresh).forEach((key) => {
        state[key] = fresh[key];
      });
    }

    function loadProgramListing(text) {
      const parsed = parseProgramListing(text);
      state.program = parsed.program;
      state.programComments = parsed.comments;
      state.pc = 0;
      state.shift = false;
      state.pendingGotoDigits = null;
      state.gotoPreview = null;
      state.pendingPrecision = false;
      return parsed;
    }

    function clearProgram() {
      state.program.fill(99);
      state.programComments = "";
      state.pc = 0;
      state.shift = false;
      state.pendingGotoDigits = null;
      state.gotoPreview = null;
      state.pendingPrecision = false;
    }

    return {
      state,
      execute,
      displayText,
      setX,
      resetExpression,
      resetCalculatorState,
      loadProgramListing,
      clearProgram,
      runOneProgramStep,
      runSingleProgramStep,
      runProgram,
      startProgram,
      stopProgram,
      pauseProgram,
      codeName,
      keyFromCode,
      roundInternal,
      makeProgramListing: () => makeProgramListing(state.program),
    };
  }

  return {
    KEY_CODES,
    PI_VALUE,
    PROGRAM_SIZE,
    createState,
    createCalculator,
    parseProgramListing,
    makeProgramListing,
    programListingLine,
    codeName,
    keyFromCode,
    isValidProgramCode,
    roundInternal,
    fitDisplayText,
    formatExponential,
  };
});
