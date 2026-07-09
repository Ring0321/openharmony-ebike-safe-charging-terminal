# RK2206 Safe-Charge Firmware Core

This folder contains the desensitized RK2206/OpenHarmony firmware core for the community e-bike safe-charging terminal.

## Contents

- `iot_smart_home_example.c`: sample entry point and task wiring.
- `BUILD.gn`: sample build description.
- `include/`: public headers for charging safety, relay control, LCD, IoT, feedback, and sensors.
- `src/`: implementation files for the safety state machine, MQTT/IoTDA messages, relay control, LCD rendering, sensor adapters, and local feedback.

## Integration Notes

Place this sample core into the RK2206 OpenHarmony sample tree that matches your board SDK. The original development target used the `e1_iot_smart_home_hwiot` sample path and the RK2206 `hb build -f` workflow.

Public source uses placeholders:

```c
YOUR_WIFI_SSID
YOUR_WIFI_PASSWORD
YOUR_IOTDA_HOST
YOUR_IOTDA_CLIENT_ID
YOUR_IOTDA_DEVICE_ID
YOUR_IOTDA_DEVICE_SECRET
```

Replace them only in a private build environment. Do not commit real Wi-Fi or IoTDA credentials.

## Demo Boundary

The project is designed around low-voltage prototype relays and sensors for a competition demo. Do not directly switch mains voltage from this sample circuit.
