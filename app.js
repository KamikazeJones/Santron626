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

const PROGRAM_STEP_DELAY_MS = 140;

const KEYS = [
  [{ key: "F", label: "F", color: "red" }, { key: "SIN", label: "sin", shift: "sin-1", color: "gray" }, { key: "COS", label: "cos", shift: "cos-1", color: "gray" }, { key: "TAN", label: "tan", shift: "tan-1", color: "gray" }, { key: "PS", label: "PS", color: "gray" }],
  [{ key: "EX", label: "e^x", shift: "ln", color: "gray" }, { key: "LOG", label: "log", shift: "10^x", color: "gray" }, { key: "Y^X", label: "y^x", color: "gray" }, { key: "STO", label: "STOn", color: "gray" }, { key: "RCL", label: "RCLn", color: "gray" }],
  [{ key: "SQRT", label: "sqrt", shift: "x^2", color: "red" }, { key: "1/X", label: "1/x", color: "red" }, { key: "X<>Y", label: "x-y", color: "blue" }, { key: "(", label: "(", color: "blue" }, { key: ")", label: ")", color: "blue" }],
  [{ key: "EXP", label: "EXP", color: "red" }, { key: "7", label: "7", color: "white" }, { key: "8", label: "8", color: "white" }, { key: "9", label: "9", color: "white" }, { key: "/", label: "÷", shift: "Mn/", color: "blue" }],
  [{ key: "PI", label: "π", color: "red" }, { key: "4", label: "4", color: "white" }, { key: "5", label: "5", color: "white" }, { key: "6", label: "6", color: "white" }, { key: "*", label: "x", shift: "Mnx", color: "blue" }],
  [{ key: "+/-", label: "+/-", color: "red" }, { key: "1", label: "1", color: "white" }, { key: "2", label: "2", color: "white" }, { key: "3", label: "3", color: "white" }, { key: "+", label: "+", shift: "Mn+", color: "blue" }],
  [{ key: "C/CE", label: "C/CE", color: "red" }, { key: "0", label: "0", color: "white" }, { key: ".", label: ".", color: "white" }, { key: "-", label: "-", shift: "Mn-", color: "blue" }, { key: "=", label: "=", color: "blue" }],
];

const state = {
  x: "0",
  y: 0,
  pending: null,
  entering: false,
  shift: false,
  pc: 0,
  program: Array(72).fill(99),
  memories: Array(10).fill(0),
  mode: "run",
  angle: "deg",
  power: "on",
  running: false,
  programPaused: false,
  runTimer: null,
};

const display = document.querySelector("#display");
const keypad = document.querySelector("#keypad");
const programList = document.querySelector("#programList");

function makeDisplay() {
  display.innerHTML = "";
  for (let i = 0; i < 12; i += 1) {
    const digit = document.createElement("div");
    digit.className = "digit";
    const block = document.createElement("div");
    block.className = "segment-block";
    "abcdefg".split("").forEach((name) => {
      const seg = document.createElement("span");
      seg.className = `seg ${name}`;
      block.append(seg);
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
    shift.textContent = item.shift || "";
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
  if (state.mode === "load") {
    const code = String(state.program[state.pc]).padStart(3, "0");
    return `${code} ${String(state.pc).padStart(2, "0")}`.padStart(12, " ");
  }
  const n = Number(state.x);
  if (!Number.isFinite(n)) return "Error".padStart(12, " ");
  if (Object.is(n, -0) || Math.abs(n) < 1e-12) return "0".padStart(12, " ");
  const abs = Math.abs(n);
  let s;
  if (abs >= 1e8 || abs < 1e-5) {
    s = n.toExponential(6).replace("e+", " ").replace("e", " ");
  } else {
    s = String(Number(n.toPrecision(9)));
  }
  return s.slice(0, 12).padStart(12, " ");
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
    digit.querySelectorAll(".seg").forEach((seg) => seg.classList.toggle("on", lit.includes(seg.classList[1])));
    digit.querySelector(".dot").classList.toggle("on", dots.slice(-12)[idx]);
  });
}

function renderProgram() {
  programList.innerHTML = "";
  state.program.forEach((code, idx) => {
    const li = document.createElement("li");
    li.className = idx === state.pc ? "active" : "";
    li.textContent = `${String(idx).padStart(2, "0")}  ${String(code).padStart(3, "0")}  ${codeName(code)}`;
    programList.append(li);
  });
}

function codeName(code) {
  const found = Object.entries(KEY_CODES).find(([, value]) => value === code);
  return found ? found[0] : code === 99 ? "END" : "";
}

function render() {
  renderDisplay(displayText());
  renderProgram();
  document.body.classList.toggle("power-off", state.power === "off");
  display.classList.toggle("power-off", state.power === "off");
}

function setX(value) {
  state.x = String(Number.isFinite(value) ? value : "Error");
  state.entering = false;
}

function pushDigit(key) {
  if (!state.entering || state.x === "0") {
    state.x = key === "." ? "0." : key;
    state.entering = true;
    return;
  }
  if (key === "." && state.x.includes(".")) return;
  if (state.x.replace("-", "").replace(".", "").length < 10) state.x += key;
}

function applyPending() {
  const x = Number(state.x);
  const y = Number(state.y);
  if (!state.pending) return x;
  if (state.pending === "+") return y + x;
  if (state.pending === "-") return y - x;
  if (state.pending === "*") return y * x;
  if (state.pending === "/") return y / x;
  if (state.pending === "Y^X") return Math.pow(y, x);
  return x;
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
    render();
    return;
  }
  if (state.mode === "load" && !fromProgram && key !== "BST" && key !== "SST") {
    state.program[state.pc] = KEY_CODES[key] ?? 99;
    state.pc = (state.pc + 1) % state.program.length;
    render();
    return;
  }
  if (state.pendingMemory && /^[0-9]$/.test(key)) {
    const idx = Number(key);
    const x = Number(state.x);
    if (state.pendingMemory === "STO") state.memories[idx] = x;
    if (state.pendingMemory === "RCL") setX(state.memories[idx]);
    if (state.pendingMemory === "M+") state.memories[idx] += x;
    if (state.pendingMemory === "M-") state.memories[idx] -= x;
    if (state.pendingMemory === "MX") state.memories[idx] *= x;
    if (state.pendingMemory === "MDIV") state.memories[idx] /= x;
    state.pendingMemory = null;
  } else if (/^[0-9.]$/.test(key)) pushDigit(key);
  else if (["+", "-", "*", "/", "Y^X"].includes(key)) {
    state.y = state.pending ? applyPending() : Number(state.x);
    state.pending = key;
    state.entering = false;
  } else if (key === "=") {
    setX(applyPending());
    state.pending = null;
  } else if (key === "C/CE") {
    state.x = "0"; state.y = 0; state.pending = null; state.entering = false;
  } else if (key === "+/-") {
    state.x = String(-Number(state.x));
  } else if (key === "PI") {
    setX(Math.PI);
  } else if (key === "X<>Y") {
    const old = Number(state.x);
    setX(state.y);
    state.y = old;
  } else if (["SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN", "LN", "LOG", "EX", "10X", "SQRT", "X2", "1/X"].includes(key)) {
    unary(key);
  } else if (key === "F") {
    state.shift = !state.shift;
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
  render();
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
  state.running = true;
  state.programPaused = false;
  scheduleProgramStep(0);
  render();
}

function pauseProgram() {
  state.running = false;
  state.programPaused = true;
  clearProgramTimer();
}

function stopProgram() {
  state.running = false;
  state.programPaused = false;
  clearProgramTimer();
  render();
}

function clearProgramTimer() {
  if (state.runTimer !== null) {
    clearTimeout(state.runTimer);
    state.runTimer = null;
  }
}

function scheduleProgramStep(delay = PROGRAM_STEP_DELAY_MS) {
  clearProgramTimer();
  state.runTimer = setTimeout(stepProgram, delay);
}

function stepProgram() {
  state.runTimer = null;
  if (!state.running || state.mode !== "run" || state.power === "off") {
    stopProgram();
    return;
  }

  const code = state.program[state.pc];
  if (code === 99) {
    stopProgram();
    return;
  }

  state.pc = (state.pc + 1) % state.program.length;
  executeProgramCode(code);

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
  const button = event.target.closest("[data-key]");
  if (!button) return;
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
});

document.querySelectorAll("input[name='mode']").forEach((input) => {
  input.addEventListener("change", () => {
    if (input.checked) state.mode = input.value;
    if (state.mode !== "run") stopProgram();
    render();
  });
});

document.querySelector("#angleDeg").addEventListener("change", () => { state.angle = "deg"; render(); });
document.querySelector("#angleRad").addEventListener("change", () => { state.angle = "rad"; render(); });
document.querySelector("#powerOn").addEventListener("change", () => { state.power = "on"; render(); });
document.querySelector("#powerOff").addEventListener("change", () => { state.power = "off"; stopProgram(); render(); });
document.querySelector("#clearProgram").addEventListener("click", () => {
  state.program.fill(99);
  state.pc = 0;
  render();
});

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
}

makeDisplay();
makeKeypad();
initializeControls();
render();
