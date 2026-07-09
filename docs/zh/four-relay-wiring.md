# OpenHarmony 社区电动车安全充电终端：四路继电器功能与接线说明

## 1. 当前四路功能

当前安全充电主板固件把 4 路继电器定义为：

| 继电器 | RK2206 GPIO | 继电器输入端 | 产品含义 | 触发场景 |
|---|---|---|---|---|
| K1 | `B2 / GPIO0_PB2` | `IN1` | 充电输出 | 授权成功、开始充电 |
| K2 | `B1 / GPIO0_PB1` | `IN2` | 散热输出 | 告警、断电保护、故障锁定、高温提醒 |
| K3 | `GPIO0_PA5` | `IN3` | 告警/占位提示 | 告警、占位、断电保护、故障锁定 |
| K4 | `B0 / GPIO0_PB0` | `IN4` | 保护锁定提示 | 断电保护、故障锁定 |

继电器模块按低电平触发处理：

```text
GPIO LOW  -> 对应继电器吸合
GPIO HIGH -> 对应继电器释放
```

## 2. 控制端接线

继电器模块控制端：

```text
继电器 VCC -> 独立 5V
继电器 GND -> 独立 5V 负极
RK2206 GND -> 继电器 GND
RK2206 B2  -> IN1
RK2206 B1  -> IN2
RK2206 PA5 -> IN3
RK2206 B0  -> IN4
```

注意：

```text
B6/B7 留给门禁板或星闪授权串口，不接继电器。
PB4/PB5/GPIO1_PD0 是 RGB 灯，不接继电器。
PC0/PC1/PC2/PC3/PA4 是 LCD，不接继电器。
```

## 3. 负载端接线

如果暂时只想看继电器板上的 K1/K2/K3/K4 指示灯，可以先不接蓝色螺丝端子的负载线。

如果要接低压负载，每一路都按同一规则：

```text
5V/12V 电源正极 -> COMx
NOx             -> 负载正极
负载负极        -> 5V/12V 电源负极
```

例如 K1：

```text
5V 正极 -> COM1
NO1     -> 小灯/电机正极
小灯/电机负极 -> 5V 负极
```

不要接 220V。比赛演示只用 5V/12V 低压小灯、小风扇、小电机。

## 4. 按键测试

| 操作 | 预期继电器动作 |
|---|---|
| 上电待机 | K1/K2/K3/K4 全部释放 |
| 按 `K3/上键` 或门禁发 `AUTH_OK` | K1 吸合 |
| 按 `K5/左键` 模拟燃气/烟雾告警 | K1 保持，K2 吸合，K3 吸合 |
| 按 `K6/右键` 断电保护 | K1 释放，K2 吸合，K3 吸合，K4 吸合 |
| 按 `K4/下键` 复位 | K1/K2/K3/K4 全部释放 |

## 5. 云端上报字段

安全充电服务 `safeCharge` 增加以下属性：

```text
relayK1Status = ON/OFF
relayK2Status = ON/OFF
relayK3Status = ON/OFF
relayK4Status = ON/OFF
relayK1Role   = CHARGE_OUTPUT
relayK2Role   = COOLING
relayK3Role   = ALERT_WARNING
relayK4Role   = PROTECT_LOCK
```

云端展示时建议把它们命名为：

```text
K1 充电输出
K2 安全散热
K3 告警提示
K4 保护锁定
```
