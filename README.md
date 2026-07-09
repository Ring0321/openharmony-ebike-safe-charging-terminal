# OpenHarmony Community E-Bike Safe Charging Terminal

OpenHarmony RK2206 smart terminal for safe community e-bike charging, with multi-port relay control, local safety protection, telemetry reporting, Huawei Cloud IoTDA integration hooks, and a northbound web dashboard.

This repository is a curated open-source version of the competition project. It keeps the reusable firmware core, dashboard source code, hardware diagrams, and Chinese project documents, while excluding local credentials, build caches, generated firmware binaries, and unrelated prototype material.

## Project Highlights

- RK2206/OpenHarmony terminal-side safety state machine for charging authorization, charging, warning, cutoff, reset, and fault handling.
- Four-relay demo model for charge output, cooling, alarm, and protection indication.
- Sensor-oriented risk logic covering temperature, smoke/gas, human presence, and current/power placeholders.
- LCD and local feedback flow for competition demonstration.
- MQTT/IoTDA reporting and command-handling code with public placeholder credentials.
- React/Vite dashboard with a local simulated backend and optional Huawei IoTDA REST integration.
- Hardware wiring notes, Draw.io diagrams, and Chinese project planning documents.

## Repository Layout

```text
firmware/rk2206-safe-charge-core/      RK2206 safe-charge firmware sample core
web/northbound-safe-charge-dashboard/  React dashboard and Node backend
docs/zh/                               Chinese planning, wiring, cloud, and status docs
docs/hardware/drawio/                  Editable hardware/system diagrams
docs/hardware/images/                  Exported hardware figure
OPEN_SOURCE_SCOPE.md                   What is included and intentionally excluded
SECURITY.md                            Secret handling and safety reporting notes
```

## Quick Start: Dashboard

```bash
cd web/northbound-safe-charge-dashboard
npm install
npm run dev
```

Open the printed local URL. Without a `.env` file, the backend runs in local simulation mode. To connect Huawei Cloud IoTDA, copy `.env.example` to `.env` and fill in your own project ID, AK/SK, endpoint, device ID, and service ID. Never commit `.env`.

## Firmware Notes

The firmware code under `firmware/rk2206-safe-charge-core` is intended to be integrated into the RK2206 OpenHarmony sample tree, typically under the `e1_iot_smart_home_hwiot` sample area. Public code uses placeholders such as `YOUR_WIFI_SSID`, `YOUR_IOTDA_HOST`, and `YOUR_IOTDA_DEVICE_SECRET`.

Generated binaries such as `Firmware.img`, `rk2206_db_loader.bin`, and MD5 files are intentionally not committed. Rebuild firmware from source in your OpenHarmony/RK2206 environment and keep device-specific secrets outside public source control.

## Safety Boundary

This project is for low-voltage prototype and competition demonstration workflows. Do not connect the demo relay circuit directly to mains voltage. Validate relay isolation, load voltage, wiring, fusing, enclosure, grounding, and local electrical regulations before any real charging scenario.

## License

Apache License 2.0. See [LICENSE](LICENSE).
