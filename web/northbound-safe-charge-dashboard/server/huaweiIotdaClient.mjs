import crypto from "node:crypto";
import { DEFAULT_DEVICE_ID, DEFAULT_SERVICE_ID } from "./defaultData.mjs";

const SIGNING_ALGORITHM = "SDK-HMAC-SHA256";

export function loadHuaweiConfig(env = process.env) {
  const endpoint = normalizeEndpoint(env.HUAWEI_IOTDA_ENDPOINT || env.IOTDA_ENDPOINT || "");
  const projectId = env.HUAWEI_PROJECT_ID || env.IOTDA_PROJECT_ID || "";
  const accessKey = env.HUAWEI_ACCESS_KEY || env.HUAWEICLOUD_SDK_AK || "";
  const secretKey = env.HUAWEI_SECRET_KEY || env.HUAWEICLOUD_SDK_SK || "";
  const deviceId = env.IOTDA_DEVICE_ID || env.HUAWEI_IOTDA_DEVICE_ID || DEFAULT_DEVICE_ID;
  const serviceId = env.IOTDA_SERVICE_ID || DEFAULT_SERVICE_ID;
  const instanceId = env.HUAWEI_IOTDA_INSTANCE_ID || env.IOTDA_INSTANCE_ID || "";

  return {
    endpoint,
    projectId,
    accessKey,
    secretKey,
    deviceId,
    serviceId,
    instanceId,
    configured: Boolean(endpoint && projectId && accessKey && secretKey && deviceId),
  };
}

export class HuaweiIotdaClient {
  constructor(config) {
    this.config = config;
  }

  async queryDeviceShadow() {
    const { projectId, deviceId } = this.config;
    return this.request("GET", `/v5/iot/${encodeURIComponent(projectId)}/devices/${encodeURIComponent(deviceId)}/shadow`);
  }

  async deliverCommand(command) {
    const { projectId, deviceId, serviceId } = this.config;
    const paras = {
      action: command.action,
      ...(command.paras || {}),
    };

    const body = {
      service_id: serviceId,
      command_name: command.command_name || "safe_charge_control",
      paras,
      expire_time: 0,
      send_strategy: "immediately",
    };

    return this.request(
      "POST",
      `/v5/iot/${encodeURIComponent(projectId)}/devices/${encodeURIComponent(deviceId)}/async-commands`,
      body,
    );
  }

  async request(method, pathname, body) {
    if (typeof fetch !== "function") {
      throw new Error("当前 Node.js 版本没有内置 fetch，请升级到 Node 18+。");
    }

    const url = new URL(pathname, this.config.endpoint);
    const payload = body === undefined ? "" : JSON.stringify(body);
    const headers = signHuaweiRequest({
      method,
      url,
      body: payload,
      accessKey: this.config.accessKey,
      secretKey: this.config.secretKey,
      instanceId: this.config.instanceId,
      hasJsonBody: body !== undefined,
    });

    const response = await fetch(url, {
      method,
      headers,
      body: body === undefined ? undefined : payload,
    });
    const text = await response.text();
    const data = text ? parseJson(text) : null;

    if (!response.ok) {
      const message = data?.error_msg || data?.message || data?.error || `${response.status} ${response.statusText}`;
      const error = new Error(`Huawei IoTDA ${method} ${pathname} failed: ${message}`);
      error.status = response.status;
      error.details = data;
      throw error;
    }
    return data;
  }
}

export function signHuaweiRequest({ method, url, body, accessKey, secretKey, instanceId, hasJsonBody }) {
  const headers = {
    Host: url.host,
    "X-Sdk-Date": formatSdkDate(new Date()),
  };
  if (hasJsonBody) headers["Content-Type"] = "application/json";
  if (instanceId) headers["Instance-Id"] = instanceId;

  const canonical = canonicalizeHeaders(headers);
  const canonicalRequest = [
    method.toUpperCase(),
    canonicalUri(url.pathname),
    canonicalQueryString(url.searchParams),
    canonical.headers,
    canonical.signedHeaders,
    sha256Hex(body || ""),
  ].join("\n");

  const stringToSign = [SIGNING_ALGORITHM, headers["X-Sdk-Date"], sha256Hex(canonicalRequest)].join("\n");
  const signature = crypto.createHmac("sha256", secretKey).update(stringToSign).digest("hex");

  return {
    ...headers,
    Authorization: `${SIGNING_ALGORITHM} Access=${accessKey}, SignedHeaders=${canonical.signedHeaders}, Signature=${signature}`,
  };
}

function normalizeEndpoint(value) {
  const trimmed = value.trim().replace(/\/+$/, "");
  if (!trimmed) return "";
  return /^https?:\/\//i.test(trimmed) ? trimmed : `https://${trimmed}`;
}

function parseJson(text) {
  try {
    return JSON.parse(text);
  } catch {
    return { raw: text };
  }
}

function canonicalizeHeaders(headers) {
  const entries = Object.entries(headers)
    .map(([key, value]) => [key.toLowerCase(), String(value).trim().replace(/\s+/g, " ")])
    .sort(([a], [b]) => a.localeCompare(b));
  return {
    headers: `${entries.map(([key, value]) => `${key}:${value}`).join("\n")}\n`,
    signedHeaders: entries.map(([key]) => key).join(";"),
  };
}

function canonicalUri(pathname) {
  return pathname
    .split("/")
    .map((segment) => encodeRfc3986(segment))
    .join("/")
    .replace(/%2F/g, "/");
}

function canonicalQueryString(searchParams) {
  return [...searchParams.entries()]
    .sort(([keyA, valueA], [keyB, valueB]) => (keyA === keyB ? valueA.localeCompare(valueB) : keyA.localeCompare(keyB)))
    .map(([key, value]) => `${encodeRfc3986(key)}=${encodeRfc3986(value)}`)
    .join("&");
}

function encodeRfc3986(value) {
  return encodeURIComponent(value).replace(/[!'()*]/g, (char) => `%${char.charCodeAt(0).toString(16).toUpperCase()}`);
}

function sha256Hex(value) {
  return crypto.createHash("sha256").update(value, "utf8").digest("hex");
}

function formatSdkDate(date) {
  const pad = (value) => String(value).padStart(2, "0");
  return [
    date.getUTCFullYear(),
    pad(date.getUTCMonth() + 1),
    pad(date.getUTCDate()),
    "T",
    pad(date.getUTCHours()),
    pad(date.getUTCMinutes()),
    pad(date.getUTCSeconds()),
    "Z",
  ].join("");
}
