# Open Source Scope

This repository is a cleaned public package of the OpenHarmony RK2206 community e-bike safe-charging project.

## Included

- Desensitized RK2206 firmware core under `firmware/rk2206-safe-charge-core`.
- Northbound React/Node dashboard under `web/northbound-safe-charge-dashboard`.
- Public `.env.example` with placeholder Huawei Cloud IoTDA fields.
- Chinese project documents under `docs/zh`.
- Editable Draw.io hardware diagrams and one exported hardware figure under `docs/hardware`.
- Apache 2.0 license, safety notes, and repository-level ignore rules.

## Excluded

- `.env` files and any real Huawei Cloud AK/SK, device secret, Wi-Fi SSID, or Wi-Fi password.
- Generated firmware images, loader binaries, MD5 files, and board-specific flashing artifacts.
- `node_modules`, `dist`, npm cache, development logs, and local runtime caches.
- Third-party installer/tool archives such as FFmpeg packages.
- Video materials, temporary Word/PDF build assets, and template extraction folders.
- Unrelated access-control prototype documents that are not part of the current safe-charging mainline.

## Current Technical Boundary

- The web dashboard can run in local simulated mode without credentials.
- Huawei Cloud IoTDA REST integration is implemented, but real cloud verification requires user-owned credentials and an online registered device.
- The firmware source still contains competition-demo assumptions and public placeholders. Replace placeholders in a private build environment only.
- INA226 real current/power measurement is documented as a next-step hardware integration item; parts of the current firmware flow still use existing sensor/demo logic.
