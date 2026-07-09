import { spawn } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";

const rootDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const node = process.execPath;
const vitePort = process.env.VITE_PORT || "5174";
const apiPort = process.env.API_PORT || "8787";
const children = [];

start("api", ["server/index.mjs"], { ...process.env, API_PORT: apiPort, NODE_ENV: "development" });
start(
  "client",
  ["node_modules/vite/bin/vite.js", "--host", "127.0.0.1", "--port", vitePort],
  { ...process.env, API_PORT: apiPort, NODE_ENV: "development" },
);

function start(name, args, env) {
  const child = spawn(node, args, {
    cwd: rootDir,
    stdio: "inherit",
    shell: false,
    env,
  });
  child.on("exit", (code) => {
    if (code && !process.exitCode) process.exitCode = code;
    stopAll(child);
  });
  children.push(child);
  console.log(`[dev:${name}] started`);
}

function stopAll(except) {
  for (const child of children) {
    if (child !== except && !child.killed) child.kill();
  }
}

process.on("SIGINT", () => {
  stopAll();
  process.exit(130);
});

process.on("SIGTERM", () => {
  stopAll();
  process.exit(143);
});
