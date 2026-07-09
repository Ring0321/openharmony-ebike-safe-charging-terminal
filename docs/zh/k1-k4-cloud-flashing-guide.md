# OpenHarmony社区电动车安全充电终端 K1-K4云端版烧录说明

## 本版固件

本版固件是烧录到 RK2206 主控板上的完整应用固件，不是单独的继电器板固件。

它保留原有社区电动车安全充电终端功能，并新增四路继电器控制与云端上报字段。

## 烧录文件

- 固件：`Firmware.img`
- Loader：`rk2206_db_loader.bin`
- 备份固件：`safe_charge_k1_k4_cloud_Firmware_20260706_221018.img`

## 四路继电器定义

| 继电器 | RK2206 引脚 | 继电器输入 | 功能 |
| --- | --- | --- | --- |
| K1 | B2 / GPIO0_PB2 | IN1 | 充电输出 |
| K2 | B1 / GPIO0_PB1 | IN2 | 散热/冷却 |
| K3 | PA5 / GPIO0_PA5 | IN3 | 告警提示 |
| K4 | B0 / GPIO0_PB0 | IN4 | 保护锁断 |

继电器控制侧需要独立 5V 供电，并与 RK2206 共地。

## 云端上报字段

- `relayK1Status`
- `relayK2Status`
- `relayK3Status`
- `relayK4Status`
- `relayK1Role`
- `relayK2Role`
- `relayK3Role`
- `relayK4Role`

角色值：

- `CHARGE_OUTPUT`
- `COOLING`
- `ALERT_WARNING`
- `PROTECT_LOCK`

## 烧录后检查

1. 继电器板 VCC 接 5V，GND 与 RK2206 共地。
2. 接线：B2->IN1，B1->IN2，PA5->IN3，B0->IN4。
3. 上电后观察继电器板指示灯或听继电器吸合声。
4. 进入不同充电状态后，在云端查看 K1-K4 状态字段是否变化。
