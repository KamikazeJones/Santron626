#!/usr/bin/env node

const fs = require("fs");

const [sourcePath, targetPath] = process.argv.slice(2);

if (!sourcePath || !targetPath) {
  console.error("Usage: node export-codex-jsonl-to-md.js <source.jsonl> <target.md>");
  process.exit(2);
}

function textFromContent(content) {
  if (typeof content === "string") return content;
  if (!Array.isArray(content)) return "";

  return content
    .map((part) => {
      if (!part || typeof part !== "object") return "";
      return part.text || part.input_text || part.output_text || "";
    })
    .filter(Boolean)
    .join("\n\n");
}

function fenceIfNeeded(text) {
  return text.replace(/\n{3,}/g, "\n\n").trim();
}

const raw = fs.readFileSync(sourcePath, "utf8");
const lines = raw.split(/\r?\n/).filter(Boolean);

let meta = null;
const messages = [];
let skippedDeveloper = 0;
let toolEvents = 0;

for (const line of lines) {
  const entry = JSON.parse(line);

  if (entry.type === "session_meta") {
    meta = entry.payload || null;
    continue;
  }

  if (entry.type !== "response_item") continue;

  const payload = entry.payload || {};
  if (payload.type === "function_call" || payload.type === "function_call_output" || payload.type === "custom_tool_call" || payload.type === "custom_tool_call_output") {
    toolEvents += 1;
    continue;
  }

  if (payload.type !== "message") continue;

  const role = payload.role;
  if (role === "developer" || role === "system") {
    skippedDeveloper += 1;
    continue;
  }

  if (role !== "user" && role !== "assistant") continue;

  const text = fenceIfNeeded(textFromContent(payload.content));
  if (!text) continue;

  messages.push({
    role,
    timestamp: entry.timestamp || "",
    text,
  });
}

const title = meta?.id || "unknown";
const started = meta?.timestamp || "";
const cwd = meta?.cwd || "";
const source = meta?.source || "";
const model = meta?.model || meta?.model_provider || "";

const out = [];
out.push(`# Codex Thread Export`);
out.push("");
out.push(`- Thread/session id: \`${title}\``);
if (started) out.push(`- Started: \`${started}\``);
if (cwd) out.push(`- Workspace: \`${cwd}\``);
if (source) out.push(`- Source: \`${source}\``);
if (model) out.push(`- Model/provider: \`${model}\``);
out.push(`- Raw events: \`${lines.length}\``);
out.push(`- Rendered chat messages: \`${messages.length}\``);
out.push(`- Tool events kept in raw JSONL: \`${toolEvents}\``);
out.push(`- Developer/system messages kept in raw JSONL: \`${skippedDeveloper}\``);
out.push("");
out.push("> This Markdown file renders the human chat only. The raw JSONL export next to it contains the full transcript, tool calls, outputs, approvals, and metadata.");
out.push("");

for (const message of messages) {
  const label = message.role === "user" ? "User" : "Assistant";
  out.push(`## ${label}`);
  if (message.timestamp) out.push(`_${message.timestamp}_`);
  out.push("");
  out.push(message.text);
  out.push("");
}

fs.writeFileSync(targetPath, `${out.join("\n")}\n`, "utf8");
