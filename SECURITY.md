# Security and Safety

## Secrets

Do not commit real credentials to this repository. Keep these values in local environment files or private deployment systems:

- Huawei Cloud access key and secret key
- IoTDA device secret
- Wi-Fi SSID and password
- Real device IDs or project IDs that should remain private

The committed `.env.example` and firmware source use placeholders only.

## Hardware Safety

This repository is for low-voltage prototype and competition demonstration work. Do not connect demo relay wiring directly to mains voltage. Before any real charging deployment, perform professional electrical review for isolation, enclosure, fusing, grounding, thermal behavior, load rating, and local regulatory compliance.

## Reporting

If you find a committed secret, unsafe wiring instruction, or security-sensitive bug, open an issue with a minimal description and avoid posting the secret itself. Rotate any exposed credential immediately in its provider console.
