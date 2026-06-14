#!/usr/bin/env node

import fs from "fs";
import path from "path";
import { chromium } from "playwright";

const DEFAULT_GUI_URL = "https://localhost:8765";

function usage(exitCode = 0) {
  console.log(`GUI test runner for Santron .test.sce files

Usage:
  node tools/playwright-gui-test.mjs [--repository] [--show-listing] <test-file>
  node tools/playwright-gui-test.mjs [--repository] [--show-listing] <url> <test-file>
  npm run gui:test -- [--repository] [--show-listing] <test-file>
  npm run gui:test -- [--repository] [--show-listing] <url> <test-file>

Notes:
  With one argument, the runner treats it as the test file and uses ${DEFAULT_GUI_URL}.
  When invoking through npm, use "--" so npm forwards the remaining arguments.
  With --repository, load commands use the GUI program list instead of direct listing fetch.
  With --show-listing, print the currently loaded program listing after the test passes.

Examples:
  node tools/playwright-gui-test.mjs tests/sanyo-cz-0911pg/mathematics/A-01_hyperbolic-functions.test.sce
  node tools/playwright-gui-test.mjs https://192.168.2.122:8765 tests/sanyo-cz-0911pg/statistics-and-probabilities/B-17_random-number-generator.test.sce
  node tools/playwright-gui-test.mjs --repository https://192.168.2.122:8765 tests/sanyo-cz-0911pg/statistics-and-probabilities/B-17_random-number-generator.test.sce
  node tools/playwright-gui-test.mjs --repository --show-listing https://192.168.2.122:8765 tests/sanyo-cz-0911pg/statistics-and-probabilities/B-17_random-number-generator.test.sce
  npm run gui:test -- tests/sanyo-cz-0911pg/mathematics/A-01_hyperbolic-functions.test.sce
  npm run gui:test -- https://192.168.2.122:8765 tests/sanyo-cz-0911pg/statistics-and-probabilities/B-17_random-number-generator.test.sce
`);
  process.exit(exitCode);
}

function parseArgs(argv) {
  if (!argv.length || argv.includes("--help") || argv.includes("-h")) usage(0);
  const args = [...argv];
  const repository = args.includes("--repository");
  const showListing = args.includes("--show-listing");
  const filtered = args.filter((arg) => arg !== "--repository" && arg !== "--show-listing");
  if (filtered.length === 1) {
    return { url: DEFAULT_GUI_URL, testFile: filtered[0], repository, showListing };
  }
  if (filtered.length === 2) {
    return { url: filtered[0], testFile: filtered[1], repository, showListing };
  }
  usage(2);
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

function parseKeys(text) {
  if (!text) return [];
  const aliases = {
    "÷": "/",
    DIV: "/",
    "×": "*",
    MUL: "*",
    YX: "Y^X",
    "Y^X": "Y^X",
    X2: "X^2",
    "X^2": "X^2",
    SQRT: "SQRT",
    "√X": "SQRT",
    EE: "EXP",
    EXP: "EXP",
    ENTER: "=",
    RUN: "R/S",
    RS: "R/S",
    "R/S": "R/S",
    GOTO: "GOTO",
    GTO: "GOTO",
    CHS: "+/-",
    MX: "M*",
    MDIV: "M/",
    EX: "E^X",
    "10X": "10^X",
  };
  const keyNames = new Map([
    "F", "SIN", "COS", "TAN", "PS", "E^X", "LOG", "Y^X", "STO", "RCL",
    "SQRT", "1/X", "X<>Y", "(", ")", "EXP", "7", "8", "9", "/",
    "PI", "4", "5", "6", "*", "+/-", "1", "2", "3", "+",
    "C/CE", "0", ".", "-", "=", "BST", "SST", "R/S", "SKP", "GOTO",
    "ASIN", "ACOS", "ATAN", "LN", "10^X", "X^2", "M+", "M-", "M*", "M/",
  ].map((key) => [key.toUpperCase(), key]));
  const parseOne = (raw) => {
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

function scenarioActions(text) {
  return normalizeScenarioText(text)
    .split(/[;\r\n]+/)
    .map((part) => part.trim())
    .filter(Boolean);
}

function listingBody(text) {
  return String(text)
    .split(/\r?\n/)
    .map((line) => line.trimEnd())
    .filter((line) => line && !line.startsWith("#"))
    .join("\n")
    .trim();
}

function formatError(message, details = {}) {
  const lines = [message];
  for (const [key, value] of Object.entries(details)) {
    lines.push(`${key}: ${value}`);
  }
  return lines.join("\n");
}

async function pageDisplay(page) {
  return page.evaluate(() => window.__santronApp.calculator.displayText().trim());
}

async function pageState(page) {
  return page.evaluate(() => ({
    running: window.__santronApp.state.running,
    mode: window.__santronApp.state.mode,
    angle: window.__santronApp.state.angle,
    comments: window.__santronApp.state.programComments,
  }));
}

async function pageListing(page) {
  return page.evaluate(() => window.__santronApp.calculator.makeProgramListing());
}

async function applySwitch(page, token) {
  const normalized = String(token).toUpperCase().replace(/^:/, "/");
  const id = {
    "/RD": "angleRad",
    "/DG": "angleDeg",
    "/LOAD": "modeLoad",
    "/CLR": "modeClear",
    "/RUN": "modeRun",
    "/ON": "powerOn",
    "/OFF": "powerOff",
  }[normalized];
  if (!id) throw new Error(`unknown switch: ${token}`);
  await page.evaluate((inputId) => {
    const input = document.getElementById(inputId);
    if (!input) throw new Error(`missing switch input ${inputId}`);
    input.checked = true;
    input.dispatchEvent(new Event("change", { bubbles: true }));
  }, id);
}

async function loadListing(page, url, filename) {
  const response = await fetch(new URL(filename, url));
  if (!response.ok) throw new Error(`listing not reachable: ${filename} (${response.status})`);
  const text = await response.text();
  await page.evaluate(({ listing, name }) => {
    window.__santronApp.applyProgramListing(listing, name);
  }, { listing: text, name: path.basename(filename) });
  return text;
}

function manifestProgramPath(filename) {
  const normalized = filename.replace(/\\/g, "/").replace(/^\.\//, "");
  if (normalized.startsWith("programs/")) return normalized.slice("programs/".length);
  if (normalized.startsWith("bin/")) return normalized.slice("bin/".length);
  return normalized;
}

async function loadListingFromRepository(page, filename) {
  const programFile = manifestProgramPath(filename);
  const targetResponse = await fetch(new URL(`programs/${programFile}`, page.url()));
  if (!targetResponse.ok) {
    throw new Error(`repository listing not reachable: programs/${programFile} (${targetResponse.status})`);
  }
  const targetListing = listingBody(await targetResponse.text());
  const loaded = await page.evaluate(async (wantedFile) => {
    const select = document.querySelector("#serverProgramSelect");
    if (!select) throw new Error("missing #serverProgramSelect");
    const response = await fetch("programs/manifest.json", { cache: "no-store" });
    if (!response.ok) throw new Error(`Programmliste konnte nicht geladen werden (${response.status})`);
    const data = await response.json();
    const programs = Array.isArray(data.programs) ? data.programs : [];
    const index = programs.findIndex((program) => program && program.file === wantedFile);
    if (index < 0) return null;
    select.value = String(index);
    select.dispatchEvent(new Event("change", { bubbles: true }));
    return programs[index]?.name || wantedFile;
  }, programFile);
  if (!loaded) {
    throw new Error(`repository program not found for load target: ${filename}`);
  }
  await page.waitForFunction((expectedListing) => {
    return window.__santronApp.calculator.makeProgramListing().trim() === expectedListing;
  }, targetListing, { timeout: 5000 });
}

async function executeKey(page, token) {
  const before = await pageState(page);
  await page.evaluate((key) => {
    window.__santronApp.execute(key);
  }, token);
  if (token === "R/S" && before.mode === "run") {
    await page.waitForFunction(() => !window.__santronApp.state.running, null, { timeout: 5000 });
  }
}

async function runScenarioInGui(page, url, testFile, text, options) {
  const actions = scenarioActions(text);
  const steps = [];
  for (let index = 0; index < actions.length; index += 1) {
    const action = actions[index];
    const expectMatch = action.match(/^expect(?:-display)?\s+(.+)$/i);
    if (expectMatch) {
      const expected = expectMatch[1].trim();
      const actual = await pageDisplay(page);
      if (actual !== expected) {
        throw new Error(formatError("expect failed", {
          file: testFile,
          step: String(index + 1),
          action,
          expected,
          actual,
        }));
      }
      steps.push(`expect ${expected}`);
      continue;
    }

      const loadMatch = action.match(/^load\s+(.+)$/i);
      if (loadMatch) {
        const filename = parseScenarioFileArgument(loadMatch[1].trim());
        if (!filename) throw new Error(`invalid load command: ${action}`);
      if (options.repository) await loadListingFromRepository(page, filename);
      else await loadListing(page, url, filename);
      steps.push(`load ${filename}`);
      continue;
    }

    if (/^list$/i.test(action) || /^save\s+/i.test(action)) {
      throw new Error(`GUI runner does not support action: ${action}`);
    }

    const keyText = action.replace(/^(keys|press)\s+/i, "");
    const tokens = parseKeys(keyText);
    for (const token of tokens) {
      if (typeof token !== "string") throw new Error(`unsupported token in GUI runner: ${JSON.stringify(token)}`);
      if (/^\/|^:/.test(token)) {
        await applySwitch(page, token);
      } else {
        await executeKey(page, token);
      }
    }
    steps.push(action);
  }
  return steps;
}

async function main() {
  const { url, testFile, repository, showListing } = parseArgs(process.argv.slice(2));
  const scenarioText = fs.readFileSync(testFile, "utf8");
  const browser = await chromium.launch();
  const context = await browser.newContext({
    ignoreHTTPSErrors: true,
    serviceWorkers: "block",
  });
  const page = await context.newPage();
  const errors = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("console", (message) => {
    if (message.type() === "error") errors.push(message.text());
  });

  try {
    await page.goto(url, { waitUntil: "networkidle" });
    await page.waitForFunction(() => !!window.__santronApp, null, { timeout: 5000 });
    await runScenarioInGui(page, url, testFile, scenarioText, { repository });
    const finalDisplay = await pageDisplay(page);
    console.log(`PASS  ${testFile}`);
    console.log(`Loaded ${url}`);
    console.log(`Load mode: ${repository ? "repository" : "direct"}`);
    console.log(`Final display: ${finalDisplay}`);
    if (showListing) {
      console.log("");
      console.log("Program listing:");
      console.log(await pageListing(page));
    }
    if (errors.length) {
      console.log("Browser errors:");
      errors.forEach((error) => console.log(error));
    }
  } finally {
    await context.close();
    await browser.close();
  }

  if (errors.length) {
    process.exitCode = 1;
  }
}

main().catch((error) => {
  console.error(error.message);
  process.exit(1);
});
