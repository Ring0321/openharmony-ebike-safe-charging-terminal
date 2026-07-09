import { cloneDefaultEvents, cloneDefaultPorts, defaultThresholds } from "./defaultData.mjs";

export function mapShadowToDashboardState(shadowResponse, previousState, meta) {
  const ports = previousState.ports.map((port) => ({ ...port }));
  const safeCharge = findService(shadowResponse?.shadow, meta.serviceId);
  const reported = safeCharge?.reported?.properties || {};
  const reportedTime = safeCharge?.reported?.event_time || null;

  if (Object.keys(reported).length > 0) {
    const portId = parsePortId(reported.portId ?? reported.port_id ?? "port_1");
    const index = ports.findIndex((port) => port.id === portId);
    const base = ports[index >= 0 ? index : 0];
    ports[index >= 0 ? index : 0] = {
      ...base,
      id: portId,
      state: mapChargeState(reported.chargeState ?? reported.state),
      riskScore: numberOr(reported.riskScore ?? reported.risk_score, base.riskScore),
      voltageV: numberOr(reported.voltageV ?? reported.pile_voltage ?? reported.voltage, base.voltageV),
      currentA: numberOr(reported.currentA ?? reported.current, base.currentA),
      powerW: numberOr(reported.powerW ?? reported.power, base.powerW),
      temperatureC: numberOr(reported.temperatureC ?? reported.temperature, base.temperatureC),
      smokeLevel: numberOr(reported.smokeLevel ?? reported.smoke, base.smokeLevel),
      relayStatus: mapOnOff(reported.relayStatus ?? reported.relay_state, "吸合", "断开", base.relayStatus),
      coolingStatus: mapOnOff(reported.coolingStatus, "运行中", "待机", base.coolingStatus),
      networkStatus: mapNetworkStatus(reported.networkStatus, base.networkStatus),
      eventTrace: String(reported.eventTrace ?? reported.lastEvent ?? base.eventTrace),
    };
  }

  const thresholds = {
    temp: numberOr(reported.temperatureLimit, previousState.thresholds.temp),
    current: numberOr(reported.currentLimit, previousState.thresholds.current),
    smoke: numberOr(reported.smokeLimit, previousState.thresholds.smoke),
  };

  const events = [...previousState.events];
  if (reported.lastEvent || reported.eventTrace) {
    events.unshift({
      time: shortTime(new Date()),
      title: "设备影子同步",
      detail: String(reported.lastEvent || reported.eventTrace),
      state: ports.some((port) => port.state === "alert") ? "warning" : "active",
    });
  }

  return {
    ...previousState,
    ports,
    thresholds,
    events: events.slice(0, 9),
    lastSync: reportedTime || new Date().toISOString(),
  };
}

export function createInitialDashboardState() {
  return {
    ports: cloneDefaultPorts(),
    events: cloneDefaultEvents(),
    thresholds: { ...defaultThresholds },
    lastSync: new Date().toISOString(),
  };
}

export function applyLocalCommand(state, command, portId) {
  const now = new Date();
  const action = command.action;
  const ports = state.ports.map((port) => ({ ...port }));
  const targetId = parsePortId(portId || "1");
  const targetIndex = Math.max(0, ports.findIndex((port) => port.id === targetId));
  const target = ports[targetIndex] || ports[0];
  let event = { title: "命令已接收", detail: `${action} 已进入本地后端队列`, state: "active" };
  let thresholds = { ...state.thresholds };

  if (action === "remote_cutoff") {
    ports[targetIndex] = {
      ...target,
      state: "cutoff",
      riskScore: Math.max(target.riskScore, 88),
      currentA: 0,
      powerW: 0,
      relayStatus: "断开",
      coolingStatus: "运行中",
      eventTrace: "远程断电命令已确认，K1 断开，K2 保持散热",
    };
    event = { title: "远程断电", detail: `${target.id}号口 K1 已断开，进入断电保护`, state: "warning" };
  }

  if (action === "reset_alarm") {
    ports[targetIndex] = {
      ...target,
      state: "idle",
      riskScore: 8,
      currentA: 0,
      powerW: 0,
      smokeLevel: 12,
      relayStatus: "断开",
      coolingStatus: "待机",
      eventTrace: "管理员复位完成，端口回到待机安全状态",
    };
    event = { title: "复位完成", detail: `${target.id}号口 已回到待机安全状态`, state: "success" };
  }

  if (action === "toggle_cooling") {
    const nextCooling = target.coolingStatus === "运行中" ? "待机" : "运行中";
    ports[targetIndex] = {
      ...target,
      coolingStatus: nextCooling,
      eventTrace: `K2 散热状态已切换为 ${nextCooling}`,
    };
    event = { title: "K2 散热切换", detail: `${target.id}号口 K2 已切换为 ${nextCooling}`, state: "active" };
  }

  if (action === "set_thresholds") {
    thresholds = {
      temp: numberOr(command.paras?.temperatureLimit, thresholds.temp),
      current: numberOr(command.paras?.currentLimit, thresholds.current),
      smoke: numberOr(command.paras?.smokeLimit, thresholds.smoke),
    };
    ports[targetIndex] = {
      ...target,
      eventTrace: `告警阈值已更新：温度 ${thresholds.temp}°C，电流 ${thresholds.current}A，烟雾 ${thresholds.smoke}ppm`,
    };
    event = { title: "阈值已更新", detail: ports[targetIndex].eventTrace, state: "success" };
  }

  return {
    ...state,
    ports,
    thresholds,
    events: [{ time: shortTime(now), ...event }, ...state.events].slice(0, 9),
    lastSync: now.toISOString(),
  };
}

function findService(shadow, serviceId) {
  if (!Array.isArray(shadow)) return null;
  return shadow.find((item) => item.service_id === serviceId) || shadow[0] || null;
}

function mapChargeState(value) {
  const normalized = String(value || "IDLE").toUpperCase();
  if (normalized === "CHARGING") return "charging";
  if (normalized === "ALERT" || normalized === "FAULT") return "alert";
  if (normalized === "CUT_OFF" || normalized === "CUTOFF") return "cutoff";
  return "idle";
}

function mapOnOff(value, onText, offText, fallback) {
  const normalized = String(value || "").toUpperCase();
  if (normalized === "ON" || normalized === "吸合" || normalized === "RUNNING") return onText;
  if (normalized === "OFF" || normalized === "断开" || normalized === "IDLE") return offText;
  return fallback;
}

function mapNetworkStatus(value, fallback) {
  const normalized = String(value || "").toUpperCase();
  if (normalized === "ONLINE") return "良好";
  if (normalized === "OFFLINE") return "离线";
  return fallback;
}

function parsePortId(value) {
  const match = String(value).match(/\d+/);
  const parsed = match ? Number(match[0]) : 1;
  if (!Number.isFinite(parsed)) return 1;
  return Math.min(4, Math.max(1, parsed));
}

function numberOr(value, fallback) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function shortTime(date) {
  return new Intl.DateTimeFormat("zh-CN", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false,
  }).format(date);
}
