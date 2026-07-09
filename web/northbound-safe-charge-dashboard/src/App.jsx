import { useCallback, useEffect, useMemo, useState } from "react";
import {
  IconActivityHeartbeat,
  IconAdjustmentsHorizontal,
  IconAlertTriangle,
  IconBell,
  IconBolt,
  IconChevronDown,
  IconCircuitCell,
  IconCloud,
  IconFlame,
  IconGauge,
  IconPower,
  IconPropeller,
  IconRefresh,
  IconShieldCheck,
  IconTemperature,
  IconUserCircle,
  IconWifi,
} from "@tabler/icons-react";
import { fetchDeviceState, sendDeviceCommand } from "./apiClient";

const statusMeta = {
  charging: { label: "充电中", tone: "success", short: "充电" },
  idle: { label: "空闲", tone: "muted", short: "空闲" },
  alert: { label: "预警中", tone: "warning", short: "预警" },
  cutoff: { label: "断电保护", tone: "danger", short: "断电" },
};

const initialPorts = [
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

const flowSteps = [
  { key: "auth", label: "授权", desc: "身份验证通过", icon: IconShieldCheck, tone: "success" },
  { key: "charge", label: "充电", desc: "继电器 K1 吸合", icon: IconBolt, tone: "success" },
  { key: "monitor", label: "监测", desc: "多维状态监测", icon: IconActivityHeartbeat, tone: "primary" },
  { key: "alert", label: "预警", desc: "风险升高", icon: IconAlertTriangle, tone: "warning" },
  { key: "cutoff", label: "断电", desc: "保护性断电", icon: IconPower, tone: "muted" },
  { key: "reset", label: "复位", desc: "人工复位恢复", icon: IconRefresh, tone: "muted" },
];

const baseEvents = [
  { time: "09:14:21", title: "授权成功", detail: "用户 · 设备授权通过", state: "success" },
  { time: "09:14:22", title: "K1 吸合", detail: "充电继电器 K1 吸合成功", state: "success" },
  { time: "09:14:23", title: "开始充电", detail: "进入充电状态，电流 3.1A，功率 698W", state: "active" },
  { time: "09:28:17", title: "风险升高", detail: "温度 58°C，电流 5.6A，风险分 82", state: "warning" },
  { time: "--:--", title: "K2 散热", detail: "散热继电器 K2 将自动吸合", state: "pending" },
  { time: "--:--", title: "断电保护", detail: "风险持续升高，系统将执行断电保护", state: "pending" },
  { time: "--:--", title: "人工复位", detail: "管理员复位后恢复充电", state: "pending" },
];

const hardwareNodes = ["华为云 IoTDA", "REST API", "MQTT", "RK2206", "K1/K2 继电器", "INA226", "MQ2", "SU-03T", "LCD"];

const defaultMeta = {
  mode: "connecting",
  iotdaConfigured: false,
  deviceOnline: false,
  deviceId: "RK2206_charge_01",
  serviceId: "safeCharge",
  latencyMs: null,
  lastSync: null,
  cloudError: null,
};

function formatAction(deviceId, action, paras = {}) {
  return {
    command_name: "safe_charge_control",
    device_id: deviceId,
    action,
    paras,
  };
}

export function App() {
  const [ports, setPorts] = useState(initialPorts);
  const [selectedId, setSelectedId] = useState(3);
  const [confirmCutoff, setConfirmCutoff] = useState(true);
  const [thresholdOpen, setThresholdOpen] = useState(false);
  const [hardwareOpen, setHardwareOpen] = useState(false);
  const [demoMode, setDemoMode] = useState(true);
  const [demoStep, setDemoStep] = useState(3);
  const [thresholds, setThresholds] = useState({ temp: 58, current: 5.8, smoke: 70 });
  const [timelineEvents, setTimelineEvents] = useState(baseEvents);
  const [backendMeta, setBackendMeta] = useState(defaultMeta);
  const [lastCommand, setLastCommand] = useState(formatAction(defaultMeta.deviceId, "remote_cutoff"));
  const [pendingAction, setPendingAction] = useState("");
  const [isRefreshing, setIsRefreshing] = useState(false);
  const [notice, setNotice] = useState({ tone: "warning", text: "正在连接本地后端 API..." });

  const applyStatePayload = useCallback((payload) => {
    if (Array.isArray(payload.ports) && payload.ports.length > 0) setPorts(payload.ports);
    if (payload.thresholds) setThresholds(normalizeThresholds(payload.thresholds));
    if (Array.isArray(payload.events) && payload.events.length > 0) setTimelineEvents(normalizeEvents(payload.events));
    setBackendMeta((current) => ({ ...current, ...pickMeta(payload) }));
  }, []);

  const loadDeviceState = useCallback(
    async ({ silent = false } = {}) => {
      if (!silent) setIsRefreshing(true);
      try {
        const payload = await fetchDeviceState();
        applyStatePayload(payload);
        if (!silent) {
          setNotice({
            tone: payload.cloudError ? "danger" : payload.iotdaConfigured ? "success" : "warning",
            text: connectionText(payload),
          });
        }
      } catch (error) {
        if (!silent) setNotice({ tone: "danger", text: `后端 API 不可用：${error.message}` });
      } finally {
        if (!silent) setIsRefreshing(false);
      }
    },
    [applyStatePayload],
  );

  useEffect(() => {
    loadDeviceState();
    const timer = window.setInterval(() => loadDeviceState({ silent: true }), 5000);
    return () => window.clearInterval(timer);
  }, [loadDeviceState]);

  useEffect(() => {
    if (!demoMode) return undefined;
    const timer = window.setInterval(() => {
      setDemoStep((step) => (step + 1) % flowSteps.length);
    }, 2800);
    return () => window.clearInterval(timer);
  }, [demoMode]);

  const selectedPort = ports.find((port) => port.id === selectedId) ?? ports[0];
  const selectedMeta = statusMeta[selectedPort.state] ?? statusMeta.idle;
  const commandBusy = Boolean(pendingAction);
  const previewCutoff = formatAction(backendMeta.deviceId, "remote_cutoff");

  const events = useMemo(
    () =>
      timelineEvents.map((event, index) => ({
        ...event,
        live: demoMode && index === Math.min(demoStep, timelineEvents.length - 1),
      })),
    [demoMode, demoStep, timelineEvents],
  );

  async function sendCommand(action, paras = {}, options = {}) {
    const command = formatAction(backendMeta.deviceId, action, paras);
    setLastCommand(command);
    setPendingAction(action);
    setNotice({ tone: "warning", text: "命令已提交，等待后端回执..." });
    try {
      const result = await sendDeviceCommand(command, selectedId);
      if (result.state) applyStatePayload(result.state);
      else await loadDeviceState({ silent: true });
      if (options.nextThresholds) setThresholds(options.nextThresholds);
      setNotice({
        tone: result.mode === "local-simulated" ? "warning" : "success",
        text:
          result.mode === "local-simulated"
            ? "未检测到 IoTDA 密钥，命令已在本地后端闭环中执行。"
            : `IoTDA 命令已下发：${result.commandResult?.status || "accepted"}`,
      });
      setConfirmCutoff(false);
      setThresholdOpen(false);
    } catch (error) {
      setNotice({ tone: "danger", text: `命令失败，状态未更新：${error.message}` });
    } finally {
      setPendingAction("");
    }
  }

  function applyThresholds(event) {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    const nextThresholds = {
      temp: Number(form.get("temp")),
      current: Number(form.get("current")),
      smoke: Number(form.get("smoke")),
    };
    sendCommand(
      "set_thresholds",
      {
        temperatureLimit: nextThresholds.temp,
        currentLimit: nextThresholds.current,
        smokeLimit: nextThresholds.smoke,
      },
      { nextThresholds },
    );
  }

  return (
    <main className="app-shell">
      <header className="topbar">
        <div className="brand">
          <span className="brand-mark">
            <IconShieldCheck size={22} stroke={2.4} />
          </span>
          <span>社区安全充电运维管理站</span>
        </div>

        <nav className="topnav" aria-label="主导航">
          {["总览", "端口", "告警", "硬件", "设置"].map((item) => (
            <button className={item === "端口" ? "nav-item active" : "nav-item"} key={item}>
              {item}
            </button>
          ))}
        </nav>

        <div className="top-actions">
          <span className={`status-pill ${backendMeta.cloudError ? "danger" : backendMeta.iotdaConfigured ? "success" : "warning"}`}>
            <IconCloud size={17} /> {backendMeta.iotdaConfigured ? (backendMeta.cloudError ? "IoTDA 异常" : "IoTDA 已接入") : "本地仿真"}
          </span>
          <span className={`status-pill ${backendMeta.deviceOnline ? "success" : "warning"}`}>
            <span className="status-dot" /> RK2206 {backendMeta.deviceOnline ? "在线" : "未在线"}
          </span>
          <span className="status-pill">
            <IconCircuitCell size={17} /> API {backendMeta.latencyMs ?? "--"}ms
          </span>
          <label className="switch">
            <span>演示模式</span>
            <input type="checkbox" checked={demoMode} onChange={() => setDemoMode((value) => !value)} />
            <i />
          </label>
          <button className="icon-button" aria-label="告警通知">
            <IconBell size={20} />
            <b>3</b>
          </button>
          <IconUserCircle className="avatar" size={28} stroke={1.8} />
        </div>
      </header>

      <section className="workspace">
        <section className="left-stage">
          <div className="intro">
            <h1>安全闭环，实时守护每一次充电</h1>
            <p>从授权到复位的全流程安全闭环，北向平台通过本地后端接入华为云 IoTDA，再把命令下沉到 RK2206。</p>
          </div>

          <div className="flow" aria-label="安全闭环流程">
            {flowSteps.map((step, index) => {
              const Icon = step.icon;
              const active = demoMode ? index === demoStep : step.key === "alert";
              return (
                <div className={`flow-step ${step.tone} ${active ? "active" : ""}`} key={step.key}>
                  <div className="flow-icon">
                    <Icon size={28} stroke={2.2} />
                  </div>
                  <strong>{step.label}</strong>
                  <span>{step.desc}</span>
                </div>
              );
            })}
          </div>

          <section className="timeline-panel">
            <div className="section-heading">
              <div>
                <h2>事件时间线</h2>
                <span>
                  {selectedPort.id}号口 · {backendMeta.mode === "iotda-rest" ? "设备影子同步" : "本地 API 闭环"}
                </span>
              </div>
              <span className="live-indicator">{isRefreshing ? "同步中" : "实时更新中"}</span>
            </div>
            <div className="timeline">
              {events.map((event, index) => (
                <article className={`timeline-row ${event.state} ${event.live ? "live" : ""}`} key={`${event.title}-${event.time}-${index}`}>
                  <span className="timeline-node" />
                  <time>{event.time}</time>
                  <strong>{event.title}</strong>
                  <p>{event.detail}</p>
                  <em>{event.state === "pending" ? "待触发" : event.state === "warning" ? "预警" : event.state === "active" ? "进行中" : "成功"}</em>
                </article>
              ))}
            </div>
            <div className="timeline-footer">
              <span>最后同步：{formatDateTime(backendMeta.lastSync)}</span>
              <span>IoTDA 服务：{backendMeta.serviceId}</span>
            </div>
          </section>
        </section>

        <aside className="right-console">
          <div className="port-tabs" role="tablist" aria-label="充电口">
            {ports.map((port) => {
              const meta = statusMeta[port.state] ?? statusMeta.idle;
              return (
                <button
                  className={`port-tab ${selectedId === port.id ? "active" : ""} ${meta.tone}`}
                  key={port.id}
                  onClick={() => {
                    setSelectedId(port.id);
                    setConfirmCutoff(false);
                  }}
                >
                  <strong>{port.id}号口</strong>
                  <span>{meta.label}</span>
                </button>
              );
            })}
          </div>

          <div className={`connection-banner ${notice.tone}`}>
            <IconCloud size={18} />
            <span>{notice.text}</span>
            <button onClick={() => loadDeviceState()} disabled={isRefreshing}>
              {isRefreshing ? "同步中" : "刷新"}
            </button>
          </div>

          <section className={`detail-card ${selectedMeta.tone}`}>
            <div className="detail-title">
              <div>
                <h2>{selectedPort.id}号口</h2>
                <span>{selectedMeta.label}</span>
              </div>
              <small>设备：{backendMeta.deviceId}</small>
            </div>

            <div className="risk-layout">
              <div className="risk-block">
                <span>风险分</span>
                <strong>{selectedPort.riskScore}</strong>
                <small>/ 100</small>
                <div className="risk-bar">
                  <i style={{ width: `${Math.min(selectedPort.riskScore, 100)}%` }} />
                </div>
              </div>
              <div className="reason-block">
                <span>预警原因</span>
                <div className="reason-tags">
                  {reasonTags(selectedPort, thresholds).map((tag) => (
                    <b key={tag}>{tag}</b>
                  ))}
                </div>
                <p>{selectedPort.eventTrace}</p>
              </div>
            </div>

            <div className="action-row">
              <button className="danger-action" onClick={() => setConfirmCutoff(true)} disabled={commandBusy}>
                <IconPower size={19} /> 远程断电
              </button>
              <button onClick={() => sendCommand("reset_alarm")} disabled={commandBusy}>
                <IconRefresh size={19} /> 复位告警
              </button>
              <button onClick={() => sendCommand("toggle_cooling")} disabled={commandBusy}>
                <IconPropeller size={19} /> {selectedPort.coolingStatus === "运行中" ? "关闭散热" : "开启散热"}
              </button>
              <button onClick={() => setThresholdOpen(true)} disabled={commandBusy}>
                <IconAdjustmentsHorizontal size={19} /> 设置阈值
              </button>
            </div>

            {confirmCutoff && (
              <div className="command-sheet">
                <button className="close-command" onClick={() => setConfirmCutoff(false)} aria-label="关闭确认">
                  ×
                </button>
                <div className="sheet-title">
                  <IconAlertTriangle size={20} />
                  <strong>远程断电命令</strong>
                </div>
                <p>执行后将切断 {selectedPort.id}号口 充电回路。后端会先下发 IoTDA 命令，失败时不会修改端口状态。</p>
                <pre>{JSON.stringify(previewCutoff, null, 2)}</pre>
                <div className="sheet-actions">
                  <button onClick={() => setConfirmCutoff(false)} disabled={commandBusy}>
                    取消
                  </button>
                  <button className="danger-action compact" onClick={() => sendCommand("remote_cutoff")} disabled={commandBusy}>
                    {pendingAction === "remote_cutoff" ? "下发中" : "确认断电"}
                  </button>
                </div>
              </div>
            )}

            <div className="sensor-grid">
              <Metric icon={IconBolt} label="电压" value={`${selectedPort.voltageV} V`} />
              <Metric icon={IconFlame} label="烟雾" value={`${selectedPort.smokeLevel} ppm`} danger={selectedPort.smokeLevel >= thresholds.smoke} />
              <Metric icon={IconGauge} label="电流" value={`${selectedPort.currentA} A`} danger={selectedPort.currentA >= thresholds.current} />
              <Metric icon={IconCircuitCell} label="K1 继电器" value={selectedPort.relayStatus} good={selectedPort.relayStatus === "吸合"} />
              <Metric icon={IconBolt} label="功率" value={`${selectedPort.powerW} W`} />
              <Metric icon={IconPropeller} label="K2 散热" value={selectedPort.coolingStatus} good={selectedPort.coolingStatus === "运行中"} />
              <Metric icon={IconTemperature} label="温度" value={`${selectedPort.temperatureC} °C`} danger={selectedPort.temperatureC >= thresholds.temp} />
              <Metric icon={IconWifi} label="网络" value={selectedPort.networkStatus} good={selectedPort.networkStatus !== "离线"} />
            </div>
          </section>

          <section className={`hardware-drawer ${hardwareOpen ? "open" : ""}`}>
            <button className="drawer-toggle" onClick={() => setHardwareOpen((value) => !value)}>
              <span>南向硬件</span>
              <small>点击展开</small>
              <IconChevronDown size={18} />
            </button>
            <div className="hardware-chain">
              {hardwareNodes.map((node, index) => (
                <span key={node}>
                  {node}
                  {index < hardwareNodes.length - 1 && <i>→</i>}
                </span>
              ))}
            </div>
            <div className="command-preview">
              <span>最近命令</span>
              <code>{JSON.stringify(lastCommand, null, 2)}</code>
            </div>
          </section>
        </aside>
      </section>

      {thresholdOpen && (
        <div className="modal-backdrop" role="dialog" aria-modal="true" aria-labelledby="threshold-title">
          <form className="threshold-modal" onSubmit={applyThresholds}>
            <div className="modal-heading">
              <h2 id="threshold-title">设置告警阈值</h2>
              <button type="button" onClick={() => setThresholdOpen(false)} aria-label="关闭阈值设置">
                ×
              </button>
            </div>
            <label>
              温度阈值 °C
              <input name="temp" type="number" step="1" defaultValue={thresholds.temp} />
            </label>
            <label>
              电流阈值 A
              <input name="current" type="number" step="0.1" defaultValue={thresholds.current} />
            </label>
            <label>
              烟雾阈值 ppm
              <input name="smoke" type="number" step="1" defaultValue={thresholds.smoke} />
            </label>
            <div className="sheet-actions">
              <button type="button" onClick={() => setThresholdOpen(false)} disabled={commandBusy}>
                取消
              </button>
              <button className="primary-action" type="submit" disabled={commandBusy}>
                {pendingAction === "set_thresholds" ? "下发中" : "保存阈值"}
              </button>
            </div>
          </form>
        </div>
      )}
    </main>
  );
}

function Metric({ icon: Icon, label, value, good = false, danger = false }) {
  return (
    <div className={`metric ${good ? "good" : ""} ${danger ? "danger" : ""}`}>
      <Icon size={18} stroke={2} />
      <span>{label}</span>
      <strong>{value}</strong>
    </div>
  );
}

function normalizeEvents(events) {
  return events.map((event) => ({
    time: event.time || "--:--",
    title: event.title || "设备事件",
    detail: event.detail || "",
    state: event.state || "active",
  }));
}

function normalizeThresholds(value) {
  return {
    temp: Number(value.temp ?? value.temperatureLimit ?? 58),
    current: Number(value.current ?? value.currentLimit ?? 5.8),
    smoke: Number(value.smoke ?? value.smokeLimit ?? 70),
  };
}

function pickMeta(payload) {
  return {
    mode: payload.mode,
    iotdaConfigured: Boolean(payload.iotdaConfigured),
    deviceOnline: Boolean(payload.deviceOnline),
    deviceId: payload.deviceId || defaultMeta.deviceId,
    serviceId: payload.serviceId || defaultMeta.serviceId,
    latencyMs: payload.latencyMs,
    lastSync: payload.lastSync,
    cloudError: payload.cloudError,
  };
}

function connectionText(payload) {
  if (payload.cloudError) return `IoTDA 调用失败，已保留后端回退状态：${payload.cloudError}`;
  if (payload.iotdaConfigured) return "IoTDA REST 已配置，页面正在读取设备影子并下发真实命令。";
  return "未配置 IoTDA AK/SK，当前使用本地后端闭环；填好 .env 后会切到真实云端。";
}

function reasonTags(port, thresholds) {
  const tags = [];
  if (port.temperatureC >= thresholds.temp) tags.push("温度升高");
  if (port.currentA >= thresholds.current) tags.push("电流异常");
  if (port.smokeLevel >= thresholds.smoke) tags.push("烟雾模拟");
  if (tags.length === 0 && port.state === "cutoff") tags.push("断电保护");
  if (tags.length === 0) tags.push("状态正常");
  return tags;
}

function formatDateTime(value) {
  if (!value) return "--";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return value;
  return new Intl.DateTimeFormat("zh-CN", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false,
  }).format(date);
}
