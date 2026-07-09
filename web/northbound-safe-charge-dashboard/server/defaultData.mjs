export const DEFAULT_DEVICE_ID = "RK2206_charge_01";
export const DEFAULT_SERVICE_ID = "safeCharge";

export const defaultThresholds = {
  temp: 58,
  current: 5.8,
  smoke: 70,
};

export const defaultPorts = [
  {
    id: 1,
    state: "charging",
    riskScore: 28,
    voltageV: 225.8,
    currentA: 3.1,
    powerW: 698,
    temperatureC: 36.4,
    smokeLevel: 18,
    relayStatus: "吸合",
    coolingStatus: "待机",
    networkStatus: "良好",
    eventTrace: "授权通过，K1 吸合，正在稳定充电",
  },
  {
    id: 2,
    state: "idle",
    riskScore: 6,
    voltageV: 0,
    currentA: 0,
    powerW: 0,
    temperatureC: 29.1,
    smokeLevel: 10,
    relayStatus: "断开",
    coolingStatus: "待机",
    networkStatus: "良好",
    eventTrace: "端口空闲，等待用户授权",
  },
  {
    id: 3,
    state: "alert",
    riskScore: 82,
    voltageV: 226.4,
    currentA: 5.62,
    powerW: 1273,
    temperatureC: 58,
    smokeLevel: 72,
    relayStatus: "吸合",
    coolingStatus: "运行中",
    networkStatus: "良好",
    eventTrace: "温度升高，电流异常，烟雾模拟触发预警",
  },
  {
    id: 4,
    state: "cutoff",
    riskScore: 91,
    voltageV: 0,
    currentA: 0,
    powerW: 0,
    temperatureC: 44.2,
    smokeLevel: 42,
    relayStatus: "断开",
    coolingStatus: "运行中",
    networkStatus: "良好",
    eventTrace: "远程断电保护已执行，等待人工复位",
  },
];

export const defaultEvents = [
  { time: "09:14:21", title: "授权成功", detail: "用户 · 设备授权通过", state: "success" },
  { time: "09:14:22", title: "K1 吸合", detail: "充电继电器 K1 吸合成功", state: "success" },
  { time: "09:14:23", title: "开始充电", detail: "进入充电状态，电流 3.1A，功率 698W", state: "active" },
  { time: "09:28:17", title: "风险升高", detail: "温度 58°C，电流 5.6A，风险分 82", state: "warning" },
  { time: "--:--", title: "K2 散热", detail: "散热继电器 K2 将自动吸合", state: "pending" },
  { time: "--:--", title: "断电保护", detail: "风险持续升高，系统将执行断电保护", state: "pending" },
  { time: "--:--", title: "人工复位", detail: "管理员复位后恢复充电", state: "pending" },
];

export function cloneDefaultPorts() {
  return defaultPorts.map((port) => ({ ...port }));
}

export function cloneDefaultEvents() {
  return defaultEvents.map((event) => ({ ...event }));
}
