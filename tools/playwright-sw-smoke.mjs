#!/usr/bin/env node

import { chromium } from "playwright";
import {
  DEFAULT_GUI_URL,
  assertSmokeResult,
  collectPageErrors,
  printSmokeResult,
  runCalculatorSmoke,
} from "./playwright-smoke-lib.mjs";

const args = new Set(process.argv.slice(2).filter((arg) => arg.startsWith("--")));
const url = process.argv.slice(2).find((arg) => !arg.startsWith("--")) || DEFAULT_GUI_URL;
const ignoreCertificateErrors = args.has("--ignore-certificate-errors");

const browser = await chromium.launch({
  args: ignoreCertificateErrors ? ["--ignore-certificate-errors"] : [],
});
const context = await browser.newContext({
  ignoreHTTPSErrors: true,
  serviceWorkers: "allow",
});
const page = await context.newPage();
const errors = collectPageErrors(page);

const result = await runCalculatorSmoke(page, url);
await page.waitForTimeout(1000);
const registrations = await page.evaluate(async () => {
  if (!navigator.serviceWorker) return [];
  return (await navigator.serviceWorker.getRegistrations()).map((registration) => ({
    scope: registration.scope,
    active: Boolean(registration.active),
    installing: Boolean(registration.installing),
    waiting: Boolean(registration.waiting),
  }));
});

await context.close();
await browser.close();

printSmokeResult(url, result);
console.log("Service workers: allow");
console.log(`Chromium ignore certificate errors: ${ignoreCertificateErrors ? "yes" : "no"}`);
console.log(`Service worker registrations: ${JSON.stringify(registrations)}`);

try {
  if (errors.length) {
    throw new Error(errors.join("\n"));
  }
  assertSmokeResult(result);
  if (!registrations.some((registration) => registration.active)) {
    throw new Error("Expected an active service worker registration");
  }
} catch (error) {
  console.error(error.message);
  process.exit(1);
}
