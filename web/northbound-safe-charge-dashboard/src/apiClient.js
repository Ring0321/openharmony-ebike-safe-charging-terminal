export async function fetchDeviceState(signal) {
  return requestJson("/api/device/state", { signal });
}

export async function sendDeviceCommand(command, portId) {
  const query = new URLSearchParams({ port_id: String(portId) });
  return requestJson(`/api/device/command?${query}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(command),
  });
}

async function requestJson(path, options = {}) {
  const response = await fetch(path, options);
  const data = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(data.error || `${response.status} ${response.statusText}`);
  }
  return data;
}
