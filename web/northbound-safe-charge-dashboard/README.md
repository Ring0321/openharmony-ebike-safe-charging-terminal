# Northbound Safe-Charge Dashboard

React/Vite dashboard plus a small Node backend for the RK2206 safe-charging terminal.

## Features

- Four-port charging status overview.
- Risk level, relay state, sensor values, and event timeline.
- Remote command UI for cutoff, alarm reset, cooling toggle, and threshold update.
- Local simulated backend for demos without cloud credentials.
- Optional Huawei Cloud IoTDA REST path for device shadow and async command delivery.

## Run Locally

```bash
npm install
npm run dev
```

The default dev server starts the API backend and Vite client together.

## Huawei IoTDA Configuration

Copy `.env.example` to `.env` and fill in your own values:

```bash
cp .env.example .env
```

Required fields:

- `HUAWEI_IOTDA_ENDPOINT`
- `HUAWEI_PROJECT_ID`
- `HUAWEI_ACCESS_KEY`
- `HUAWEI_SECRET_KEY`
- `IOTDA_DEVICE_ID`
- `IOTDA_SERVICE_ID`

Keep `.env` private. It is ignored by the repository.

## Build

```bash
npm run build
```

The generated `dist/` folder is intentionally ignored.
