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

const Core = window.SantronCore;
const calculator = Core.createCalculator();
const { state } = calculator;

const KEYS = [
  [{ key: "F", label: "F", color: "red" }, { key: "SIN", label: "sin", shift: "sin^-1", color: "gray" }, { key: "COS", label: "cos", shift: "cos^-1", color: "gray" }, { key: "TAN", label: "tan", shift: "tan^-1", color: "gray" }, { key: "PS", label: "PS", color: "gray" }],
  [{ key: "E^X", label: "e^x", shift: "ln", color: "gray" }, { key: "LOG", label: "log", shift: "10^x", color: "gray" }, { key: "Y^X", label: "y^x", color: "gray" }, { key: "STO", label: "STOn", color: "gray" }, { key: "RCL", label: "RCLn", color: "gray" }],
  [{ key: "SQRT", label: "√x", shift: "X^2", color: "red" }, { key: "1/X", label: "1/x", color: "red" }, { key: "X<>Y", label: "x-y", color: "blue" }, { key: "(", label: "(", color: "blue" }, { key: ")", label: ")", color: "blue" }],
  [{ key: "EXP", label: "EXP", color: "red" }, { key: "7", label: "7", color: "white" }, { key: "8", label: "8", color: "white" }, { key: "9", label: "9", color: "white" }, { key: "/", label: "÷", shift: "Mn÷", color: "blue" }],
  [{ key: "PI", label: "π", color: "red" }, { key: "4", label: "4", color: "white" }, { key: "5", label: "5", color: "white" }, { key: "6", label: "6", color: "white" }, { key: "*", label: "×", shift: "Mn×", color: "blue" }],
  [{ key: "+/-", label: "+/-", color: "red" }, { key: "1", label: "1", color: "white" }, { key: "2", label: "2", color: "white" }, { key: "3", label: "3", color: "white" }, { key: "+", label: "+", shift: "Mn+", color: "blue" }],
  [{ key: "C/CE", label: "C/CE", color: "red" }, { key: "0", label: "0", color: "white" }, { key: ".", label: ".", color: "white" }, { key: "-", label: "-", shift: "Mn-", color: "blue" }, { key: "=", label: "=", color: "blue" }],
];

Object.assign(state, {
  runSpeed: 0,
  runDisplayMode: "blank",
  heldRunDisplay: null,
  runTimer: null,
});

const display = document.querySelector("#display");
const keypad = document.querySelector("#keypad");
const programList = document.querySelector("#programList");
const programComments = document.querySelector("#programComments");
const programMemoryTab = document.querySelector("#programMemoryTab");
const programCommentTab = document.querySelector("#programCommentTab");
const programMemoryPanel = document.querySelector("#programMemoryPanel");
const programCommentPanel = document.querySelector("#programCommentPanel");
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
let displayCells = [];
let lastRenderedDisplay = null;

function makeDisplay() {
  display.innerHTML = "";
  displayCells = [];
  for (let i = 0; i < 12; i += 1) {
    const digit = document.createElement("div");
    digit.className = "digit";
    const block = document.createElement("div");
    block.className = "segment-block";
    const segments = {};
    const glows = {};
    "abcdefg".split("").forEach((name) => {
      const wrap = document.createElement("div");
      wrap.className = `seg-glow ${name}`;
      const seg = document.createElement("span");
      seg.className = `seg ${name}`;
      segments[name] = seg;
      glows[name] = wrap;
      wrap.append(seg);
      block.append(wrap);
    });
    const dot = document.createElement("span");
    dot.className = "dot";
    digit.append(block, dot);
    display.append(digit);
    displayCells.push({ block, dot, segments, glows, char: null, dotOn: null });
  }
  lastRenderedDisplay = null;
}

function makeKeypad() {
  keypad.innerHTML = "";
  KEYS.flat().forEach((item) => {
    const wrap = document.createElement("div");
    wrap.className = "key-wrap";
    const shift = document.createElement("div");
    shift.className = "shift";
    if (item.shift) {
      shift.innerHTML = keyLabelHtml(item.shift);
    }
    const button = document.createElement("button");
    button.className = `pill ${item.color}`;
    if (item.label.length >= 4) button.classList.add("long-label");
    button.type = "button";
    button.dataset.key = item.key;
    button.innerHTML = keyLabelHtml(item.label);
    wrap.append(shift, button);
    keypad.append(wrap);
  });
}

function keyLabelHtml(text) {
  return text
    .replace(/√x/g, '<span class="root-symbol"><span class="root-mark">√</span><span class="root-radicand">x</span></span>')
    .replace(/([A-Za-z0-9]+)\^(-1)/g, '$1<sup class="inverse-exp">$2</sup>')
    .replace(/([A-Za-z0-9]+)\^(-?\d+)/g, '$1<sup class="power-exp numeric-exp">$2</sup>')
    .replace(/([A-Za-z0-9]+)\^([A-Za-z])/g, '$1<sup class="power-exp">$2</sup>')
    .replace(/π/g, '<span class="pi-symbol">π</span>')
    .replace(/[×÷]/g, (symbol) => `<span class="operator-symbol">${symbol}</span>`);
}

function displayText() {
  if (state.power === "off") return " ".repeat(12);
  if (state.running && state.runDisplayMode === "blank") return " ".repeat(12);
  if (state.running && state.runDisplayMode === "hold" && state.heldRunDisplay !== null) return state.heldRunDisplay;
  return normalDisplayText();
}

function normalDisplayText() {
  return calculator.displayText();
}

function renderDisplay(text) {
  if (text === lastRenderedDisplay) return;
  lastRenderedDisplay = text;

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
  const visibleChars = chars.slice(-12);
  const visibleDots = dots.slice(-12);
  visibleChars.forEach((ch, idx) => {
    const cell = displayCells[idx];
    const dotOn = visibleDots[idx];
    if (cell.char === ch && cell.dotOn === dotOn) return;

    const lit = SEGMENTS[ch] || "";
    cell.block.classList.toggle("lit", lit.length > 0);
    Object.entries(cell.segments).forEach(([name, seg]) => {
      const on = lit.includes(name);
      seg.classList.toggle("on", on);
      cell.glows[name].classList.toggle("on", on);
    });
    cell.dot.classList.toggle("on", dotOn);
    cell.char = ch;
    cell.dotOn = dotOn;
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

function renderProgramComments() {
  if (!programComments) return;
  programComments.textContent = state.programComments || "Keine Kommentare.";
}

function markProgramRenderDirty() {
  programRenderDirty = true;
}

function codeName(code) {
  return Core.codeName(code);
}

function programListingLine(code, idx) {
  const address = String(idx).padStart(2, "0");
  const keyCode = String(code).padStart(3, "0");
  const name = codeName(code);
  return `${address} ${keyCode} ${name}`.trimEnd();
}

function makeProgramListing() {
  return calculator.makeProgramListing();
}

function isValidProgramCode(code) {
  return Core.isValidProgramCode(code);
}

function parseProgramListing(text) {
  return Core.parseProgramListing(text);
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
  calculator.loadProgramListing(text);
  if (programName) programName.value = programNameFromFile(name);
  markProgramRenderDirty();
  renderProgramComments();
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
  } catch (error) {
    alert(error.message);
  }
}

function resetCalculatorState() {
  clearProgramTimer();
  calculator.resetCalculatorState();
  state.runSpeed = runSpeedFromSlider(runSpeed.value);
  state.runDisplayMode = document.querySelector("input[name='runDisplayMode']:checked")?.value || "blank";
  state.heldRunDisplay = null;
  state.runTimer = null;
  markProgramRenderDirty();
  renderProgramComments();
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
  const wasRunning = state.running;
  const stillRunning = calculator.runOneProgramStep();
  if (wasRunning && !state.running) {
    state.heldRunDisplay = null;
    clearProgramTimer();
    runDisplayBlanked = false;
    runDisplayHeld = false;
  }
  markProgramRenderDirty();
  return stillRunning;
}

function runSingleProgramStep() {
  calculator.runSingleProgramStep();
  markProgramRenderDirty();
  render();
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

  render();
  if (state.running) {
    scheduleProgramStep();
  }
}

document.addEventListener("click", (event) => {
  if (performance.now() - lastPointerKeyTime < 500) return;
  const button = event.target.closest("[data-key]");
  if (!button) return;
  pressKeyButton(button);
});

document.addEventListener("pointerdown", (event) => {
  const button = event.target.closest("[data-key]");
  if (!button) return;
  vibrateKeyFeedback();
  event.preventDefault();
  lastPointerKeyTime = performance.now();
  pressKeyButton(button);
});

document.addEventListener("pointerup", (event) => {
  const button = event.target.closest("[data-key]");
  if (!button) return;
  event.preventDefault();
});

function pressKeyButton(button) {
  showKeyPress(button);
  let key = normalizeGuiKey(button.dataset.key);
  if (state.shift) {
    const shifted = {
      SIN: "ASIN", COS: "ACOS", TAN: "ATAN", "E^X": "LN", LOG: "10^X", SQRT: "X^2",
      "+": "M+", "-": "M-", "*": "M*", "/": "M/",
    };
    key = shifted[key] || key;
    state.shift = false;
  }
  execute(key);
}

function normalizeGuiKey(key) {
  if (key === "GTO") return "GOTO";
  if (key === "X2") return "X^2";
  if (key === "EX") return "E^X";
  if (key === "10X") return "10^X";
  if (key === "MX") return "M*";
  if (key === "MDIV") return "M/";
  return key;
}

function execute(key, fromProgram = false) {
  const normalized = normalizeGuiKey(key);
  if (!fromProgram && normalized === "R/S" && state.mode === "run") {
    toggleProgramRun();
    return;
  }
  if (!fromProgram && normalized === "SST" && state.mode === "run") {
    calculator.runSingleProgramStep();
    markProgramRenderDirty();
    render();
    return;
  }
  calculator.execute(normalized, fromProgram);
  markProgramRenderDirty();
  if (!fromProgram) render();
}

function startProgram() {
  if (state.running || state.power === "off") return;
  state.heldRunDisplay = normalDisplayText();
  calculator.startProgram();
  scheduleProgramStep(0);
  render();
}

function pauseProgram() {
  calculator.pauseProgram();
  state.heldRunDisplay = null;
  clearProgramTimer();
  render();
}

function stopProgram() {
  calculator.stopProgram();
  state.heldRunDisplay = null;
  clearProgramTimer();
  render();
}

function toggleProgramRun() {
  if (state.running) stopProgram();
  else startProgram();
}

function showKeyPress(button) {
  button.classList.add("is-pressed");
  window.setTimeout(() => {
    button.classList.remove("is-pressed");
  }, 85);
}

function vibrateKeyFeedback() {
  if (!("vibrate" in navigator)) return;
  navigator.vibrate([18]);
}

document.querySelectorAll("input[name='mode']").forEach((input) => {
  input.addEventListener("change", () => {
    if (input.checked) state.mode = input.value;
    if (state.mode !== "run") stopProgram();
    state.pendingGotoDigits = null;
    state.gotoPreview = null;
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
  calculator.clearProgram();
  markProgramRenderDirty();
  renderProgramComments();
  render();
});

function selectProgramTab(tabName) {
  const comments = tabName === "comments";
  programMemoryTab.classList.toggle("active", !comments);
  programCommentTab.classList.toggle("active", comments);
  programMemoryTab.setAttribute("aria-selected", String(!comments));
  programCommentTab.setAttribute("aria-selected", String(comments));
  programMemoryPanel.classList.toggle("active", !comments);
  programCommentPanel.classList.toggle("active", comments);
}

programMemoryTab.addEventListener("click", () => selectProgramTab("memory"));
programCommentTab.addEventListener("click", () => selectProgramTab("comments"));

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
  calculator.resetExpression();
  state.pendingPrecision = false;
  state.pendingGotoDigits = null;
  state.gotoPreview = null;
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

function registerServiceWorker() {
  if (!("serviceWorker" in navigator)) return;
  window.addEventListener("load", () => {
    navigator.serviceWorker.register("sw.js").catch(() => {
      // The emulator still works when install support is unavailable.
    });
  });
}

makeDisplay();
makeKeypad();
initializeControls();
initializeServerPrograms();
renderPower();
renderProgramComments();
renderNow();
showMobileCalculator();
registerServiceWorker();
window.addEventListener("pageshow", showMobileCalculator);

window.__santronApp = {
  calculator,
  state,
  execute,
  applyProgramListing,
  resetCalculatorState,
  selectProgramTab,
};
