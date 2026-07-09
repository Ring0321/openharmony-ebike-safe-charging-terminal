import fs from "node:fs";
import http from "node:http";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { DEFAULT_DEVICE_ID, DEFAULT_SERVICE_ID } from "./defaultData.mjs";
import { loadDotEnv } from "./env.mjs";
import { HuaweiIotdaClient, loadHuaweiConfig } from "./huaweiIotdaClient.mjs";
import { applyLocalCommand, createInitialDashboardState, mapShadowToDashboardState } from "./stateMapper.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const rootDir = path.resolve(__dirname, "..");
loadDotEnv(rootDir);

const PORT = Number(process.env.API_PORT || process.env.PORT || 8787);
const config = loadHuaweiConfig(process.env);
const useIotda = config.configured && process.env.IOTDA_MODE !== "local";
const iotdaClient = useIotda ? new HuaweiIotdaClient(config) : null;
let dashboardState = createInitialDashboardState();
let lastCloudError = null;
let lastLatencyMs = null;

const server = http.createServer(async (request, response) => {
  try {
    addCorsHeaders(request, response);
    if (request.method === "OPTIONS") {
      response.writeHead(204);
      response.end();
      return;
    }

    const url = new URL(request.url || "/", `http://${request.headers.host || "127.0.0.1"}`);
    if (url.pathname === "/api/health") {
      sendJson(response, 200, buildMeta({ ok: true }));
      return;
    }
    if (url.pathname === "/api/device/state" && request.method === "GET") {
      const state = await readDeviceState();
      sendJson(response, 200, state);
      return;
    }
    if (url.pathname === "/api/events" && request.method === "GET") {
      sendJson(response, 200, { events: dashboardState.events, ...buildMeta() });
      return;
    }
    if (url.pathname === "/api/device/command" && request.method === "POST") {
      const body = await readJsonBody(request);
      const portId = url.searchParams.get("port_id") || url.searchParams.get("portId") || "1";
      const result = await handleDeviceCommand(body, portId);
      sendJson(response, 200, result);
      return;
    }

    if (serveStatic(request, response, url.pathname)) return;
    sendJson(response, 404, { error: "Not found" });
  } catch (error) {
    const status = error.status && Number.isInteger(error.status) ? error.status : 500;
    sendJson(response, status, {
      error: error.message || "Server error",
      details: error.details || null,
      ...buildMeta({ ok: false }),
    });
  }
});

server.listen(PORT, "127.0.0.1", () => {
  console.log(`Safe charge API listening on http://127.0.0.1:${PORT}`);
  console.log(useIotda ? `IoTDA REST enabled for ${config.deviceId}` : "IoTDA REST not configured; using local closed-loop simulation.");
});

async function readDeviceState() {
  if (!iotdaClient) {
    return {
      ports: dashboardState.ports,
      thresholds: dashboardState.thresholds,
      events: dashboardState.events,
      ...buildMeta({ mode: "local-simulated" }),
    };
  }

  const started = Date.now();
  try {
    const shadow = await iotdaClient.queryDeviceShadow();
    lastLatencyMs = Date.now() - started;
    lastCloudError = null;
    dashboardState = mapShadowToDashboardState(shadow, dashboardState, config);
    return {
      ports: dashboardState.ports,
      thresholds: dashboardState.thresholds,
      events: dashboardState.events,
      rawShadow: process.env.EXPOSE_RAW_IOTDA_SHADOW === "true" ? shadow : undefined,
      ...buildMeta({ mode: "iotda-rest" }),
    };
  } catch (error) {
    lastLatencyMs = Date.now() - started;
    lastCloudError = error.message;
    return {
      ports: dashboardState.ports,
      thresholds: dashboardState.thresholds,
      events: dashboardState.events,
      ...buildMeta({ mode: "iotda-error" }),
    };
  }
}

async function handleDeviceCommand(body, portId) {
  const command = normalizeCommand(body);
  if (!iotdaClient) {
    dashboardState = applyLocalCommand(dashboardState, command, portId);
    return {
      ok: true,
      command,
      commandResult: {
        status: "LOCAL_APPLIED",
        message: "未配置 IoTDA 密钥，命令已在本地后端闭环中执行。",
      },
      state: {
        ports: dashboardState.ports,
        thresholds: dashboardState.thresholds,
        events: dashboardState.events,
      },
      ...buildMeta({ mode: "local-simulated" }),
    };
  }

  const started = Date.now();
  try {
    const commandResult = await iotdaClient.deliverCommand(command);
    lastLatencyMs = Date.now() - started;
    lastCloudError = null;
    const state = await readDeviceState();
    return {
      ok: true,
      command,
      commandResult,
      state: {
        ports: state.ports,
        thresholds: state.thresholds,
        events: state.events,
      },
      ...buildMeta({ mode: "iotda-rest" }),
    };
  } catch (error) {
    lastLatencyMs = Date.now() - started;
    lastCloudError = error.message;
    error.status = error.status || 502;
    throw error;
  }
}

function normalizeCommand(body) {
  if (!body || typeof body !== "object") {
    const error = new Error("命令体必须是 JSON 对象。");
    error.status = 400;
    throw error;
  }

  const commandName = body.command_name || "safe_charge_control";
  const action = body.action || body.paras?.action;
  if (commandName !== "safe_charge_control") {
    const error = new Error("仅支持 safe_charge_control 命令。");
    error.status = 400;
    throw error;
  }
  if (!["remote_cutoff", "reset_alarm", "toggle_cooling", "set_thresholds"].includes(action)) {
    const error = new Error(`不支持的 action：${action || "(empty)"}`);
    error.status = 400;
    throw error;
  }

  return {
    command_name: commandName,
    device_id: body.device_id || config.deviceId || DEFAULT_DEVICE_ID,
    action,
    paras: body.paras || {},
  };
}

function buildMeta(extra = {}) {
  const deviceOnline = dashboardState.ports.some((port) => port.networkStatus !== "离线");
  return {
    ok: extra.ok ?? !lastCloudError,
    mode: extra.mode || (useIotda ? "iotda-rest" : "local-simulated"),
    iotdaConfigured: config.configured,
    serviceId: config.serviceId || DEFAULT_SERVICE_ID,
    deviceId: config.deviceId || DEFAULT_DEVICE_ID,
    deviceOnline,
    latencyMs: lastLatencyMs,
    lastSync: dashboardState.lastSync,
    cloudError: lastCloudError,
  };
}

function readJsonBody(request) {
  return new Promise((resolve, reject) => {
    let body = "";
    request.on("data", (chunk) => {
      body += chunk;
      if (body.length > 1024 * 1024) {
        reject(new Error("请求体过大。"));
        request.destroy();
      }
    });
    request.on("end", () => {
      try {
        resolve(body ? JSON.parse(body) : {});
      } catch {
        const error = new Error("请求体不是有效 JSON。");
        error.status = 400;
        reject(error);
      }
    });
    request.on("error", reject);
  });
}

function sendJson(response, status, data) {
  response.writeHead(status, { "Content-Type": "application/json; charset=utf-8" });
  response.end(JSON.stringify(data));
}

function addCorsHeaders(request, response) {
  const origin = request.headers.origin;
  if (origin && /^https?:\/\/(127\.0\.0\.1|localhost)(:\d+)?$/i.test(origin)) {
    response.setHeader("Access-Control-Allow-Origin", origin);
    response.setHeader("Vary", "Origin");
  }
  response.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  response.setHeader("Access-Control-Allow-Headers", "Content-Type");
}

function serveStatic(request, response, pathname) {
  const distDir = path.join(rootDir, "dist");
  if (!fs.existsSync(distDir) || request.method !== "GET") return false;

  const requestedPath = pathname === "/" ? "/index.html" : pathname;
  const filePath = path.resolve(distDir, `.${decodeURIComponent(requestedPath)}`);
  if (!filePath.startsWith(distDir)) {
    sendJson(response, 403, { error: "Forbidden" });
    return true;
  }

  const finalPath = fs.existsSync(filePath) && fs.statSync(filePath).isFile() ? filePath : path.join(distDir, "index.html");
  const contentType = contentTypeFor(finalPath);
  response.writeHead(200, { "Content-Type": contentType });
  fs.createReadStream(finalPath).pipe(response);
  return true;
}

function contentTypeFor(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  if (ext === ".html") return "text/html; charset=utf-8";
  if (ext === ".js") return "text/javascript; charset=utf-8";
  if (ext === ".css") return "text/css; charset=utf-8";
  if (ext === ".svg") return "image/svg+xml";
  if (ext === ".png") return "image/png";
  return "application/octet-stream";
}
