#!/usr/bin/env node

import { chromium } from "playwright";
import {
  DEFAULT_GUI_URL,
  assertSmokeResult,
  collectPageErrors,
  printSmokeResult,
  runCalculatorSmoke,
} from "./playwright-smoke-lib.mjs";

const url = process.argv[2] || DEFAULT_GUI_URL;
const browser = await chromium.launch();
const context = await browser.newContext({
  ignoreHTTPSErrors: true,
  serviceWorkers: "block",
});
const page = await context.newPage();
const errors = collectPageErrors(page);
const result = await runCalculatorSmoke(page, url);

await context.close();
await browser.close();

if (errors.length) {
  console.error(errors.join("\n"));
  process.exit(1);
}

printSmokeResult(url, result);

try {
  assertSmokeResult(result);
} catch (error) {
  console.error(error.message);
  process.exit(1);
}
