# OpenHarmony 社区电动车安全充电比赛项目复用总结

用途：把本项目的思路、硬件、软件环境、操作工具和开发步骤整理成一份可复用模板，给另一个嵌入式比赛项目或另一个 Codex 工作线程效仿。

更新时间：2026-07-06

## 1. 核心思路

这个项目的主线不是直接做真实市电充电桩，而是用 RK2206 OpenHarmony 开发板做一个“低压安全充电终端原型”。

比赛展示重点是：

1. 端侧真实运行 OpenHarmony。
2. RK2206 通过 GPIO 控制继电器，让低压负载真实通断。
3. RK2206 读取传感器或模拟数据，进入安全充电状态机。
4. LCD、RGB、蜂鸣、语音、风扇/电机让评委直观看到状态变化。
5. WiFi/MQTT 把端侧状态同步到云端或后台。
6. 整个流程围绕“预约、认证、预检、充电、告警、断电、复位、追溯”闭环展开。

一句话总结：

> 先把南向硬件闭环做稳，再把云端、后台、AI、数字孪生作为展示和监管能力接上去。

## 2. 可效仿的项目结构

另一个项目可以直接套用这个结构：

```text
场景问题
-> 主控开发板
-> 传感器输入
-> 执行器输出
-> 本地状态机
-> 屏幕/灯光/声音反馈
-> MQTT/云端监管
-> 比赛展示文档和演示流程
```

本项目对应关系：

| 层级 | 本项目做法 | 另一个项目效仿方式 |
|---|---|---|
| 场景 | 社区电动车安全充电 | 换成门禁、仓储、消防、农业、工控等场景 |
| 主控 | RK2206 OpenHarmony 开发板 | 保持 RK2206 或换成目标比赛指定板卡 |
| 输入 | 温湿度、人体、烟雾/燃气、电流功率、认证串口 | 换成新项目需要的传感器 |
| 输出 | K1 充电继电器、K2 散热继电器、RGB、蜂鸣、LCD、语音 | 换成新项目的执行器 |
| 业务 | 安全充电状态机 | 改成新项目状态机 |
| 云端 | MQTT / 华为云 IoTDA safeCharge 服务 | 改成新服务模型和属性 |
| 展示 | 低压负载、状态变化、断电保护 | 做可见、可听、可复现的演示动作 |

## 3. 当前工程和资料位置

文档资料目录：

```text
local-project-workspace
```

真实 OpenHarmony 源码工程目录：

```text
path/to/txsmartropenharmony/vendor/isoftstone/rk2206/samples/e1_iot_smart_home_hwiot
```

重点源码文件：

| 文件 | 作用 |
|---|---|
| `iot_smart_home_example.c` | 应用入口，初始化传感器、状态机、联网、认证线程 |
| `src/charge_safety.c` | 安全充电状态机和命令处理 |
| `include/charge_safety.h` | 状态、事件、风险等级、命令枚举 |
| `src/relay_control.c` | K1/K2 继电器 GPIO 控制 |
| `src/power_sensor.c` | INA219 功率检测，失败时使用模拟功率数据 |
| `src/iot.c` | MQTT 上报和命令下发 |
| `src/smart_home.c` | LCD 显示、按键命令、演示逻辑 |
| `src/access_auth.c` | 串口认证输入，接门锁板或 LZ3663 通信板 |
| `src/buzzer_control.c` | 蜂鸣器告警 |
| `src/drv_light.c` | RGB/灯光反馈 |
| `src/drv_motor.c` | 电机反馈 |

注意：当前公开仓库里的 `txsmartropenharmony` 主要是说明和框架，不是完整 SDK 源码。做代码修改时，以你本机实际 OpenHarmony SDK 中的 `.../e1_iot_smart_home_hwiot` 为准。

## 4. 硬件清单

### 4.1 必备硬件

| 硬件 | 数量 | 用法 | 效仿意义 |
|---|---:|---|---|
| RK2206 OpenHarmony 综合开发板 | 1 | 主控、LCD、按键、WiFi/MQTT、状态机 | 比赛作品的端侧核心 |
| 4 路 5V 光耦继电器模块，低电平触发优先 | 1 | K1 控制充电输出，K2 控制散热风扇，后续扩到 4 口 | 把软件状态变成真实硬件动作 |
| 独立 5V 电源或面包板电源模块 | 1 | 给继电器和低压负载供电 | 避免 RK2206 被负载拉垮 |
| 5V/12V 小灯、小风扇、小电机或电阻负载 | 若干 | 模拟充电负载、散热动作 | 让评委直接看到通断 |
| 杜邦线、面包板、端子排 | 若干 | 接线、调试和展示固定 | 降低现场排查难度 |

### 4.2 推荐硬件

| 硬件 | 用法 |
|---|---|
| INA219 电流功率检测模块 | 当前源码已支持 INA219，未接入时自动模拟电压、电流、功率 |
| INA226/CJMCU-226 | 产品计划里的升级目标，适合作为更精确的 1 号口测量模块 |
| 有源蜂鸣器模块 | 告警时发声 |
| RGB 灯或灯带 | 显示待机、充电、告警、断电等状态 |
| SU-03T 语音模块 | 播报“开始充电、异常告警、保护断电”等 |
| 亚克力展示板 | 把主板、继电器、负载、接线图固定成比赛展板 |

### 4.3 后期扩展硬件

| 硬件 | 作用 |
|---|---|
| 紫色智能门锁板 | 做刷卡、指纹、密码等认证终端 |
| LZ3663 串口/无线通信板 | 把认证结果转发给 RK2206 |
| RFID、指纹模块、数字键盘 | 做身份认证 |
| TCA9548A I2C 复用模块 | 后续扩多路 INA226 |
| 多路插枪检测微动开关 | 做端口插入/拔出识别 |

## 5. 当前有效硬件接线

### 5.1 重要版本说明

早期文档里出现过 `GPIO0_PA5` 作为继电器预留口。当前有效源码和接线指导已经改为：

| 功能 | 当前接口 |
|---|---|
| K1 充电输出继电器 | `B2 / GPIO0_PB2 -> IN1` |
| K2 散热风扇继电器 | `B1 / GPIO0_PB1 -> IN2` |
| 串口认证 RX | `B6 / GPIO0_PB6 / UART0_RX` |
| 串口认证 TX | `B7 / GPIO0_PB7 / UART0_TX` |

所以，另一个项目照搬时，一定要以当前源码 `src/relay_control.c` 和 `src/access_auth.c` 为准。

### 5.2 K1 充电输出继电器接法

控制侧：

```text
RK2206 B2 / GPIO0_PB2  -> 继电器 IN1
RK2206 GND             -> 继电器 GND
外部 5V +              -> 继电器 VCC
外部 5V -              -> 继电器 GND
```

负载侧：

```text
外部电源正极
-> 继电器 COM1
-> 继电器 NO1
-> 低压负载正极
低压负载负极
-> 外部电源负极
```

低电平触发逻辑：

```text
GPIO0_PB2 = LOW  -> K1 吸合 -> 低压负载通电
GPIO0_PB2 = HIGH -> K1 释放 -> 低压负载断电
```

源码中对应配置：

```c
#define SAFE_CHARGE_RELAY_ENABLE_GPIO 1
#define SAFE_CHARGE_RELAY_GPIO GPIO0_PB2
#define SAFE_CHARGE_RELAY_ACTIVE_LOW 1
```

### 5.3 K2 散热风扇继电器接法

```text
RK2206 B1 / GPIO0_PB1 -> 继电器 IN2
继电器 COM2/NO2       -> 小风扇或散热负载
```

K2 的产品含义不是“充电输出”，而是“异常或断电后继续散热”。演示时更像真实设备：K1 断开以后，K2 仍可保持散热一段时间。

源码中对应配置：

```c
#define SAFE_CHARGE_COOLING_RELAY_ENABLE_GPIO 1
#define SAFE_CHARGE_COOLING_RELAY_GPIO GPIO0_PB1
```

### 5.4 INA219 / INA226 功率检测接法

当前源码实际支持 INA219：

```text
INA219 VCC -> RK2206 3.3V
INA219 GND -> RK2206 GND，并与负载电源负极共地
INA219 SCL -> RK2206 I2C SCL
INA219 SDA -> RK2206 I2C SDA
负载电流路径 -> 经过 INA219 采样端
```

当前策略：

1. 没有接 INA219 时，`power_sensor.c` 会使用模拟数据，保证演示不断。
2. 接入 INA219 后，LCD 和 MQTT 里的 `powerSensor` 应从 `SIMULATED` 变成 `INA219`。
3. 产品计划里可以写 INA226 作为升级方向，但当前开发和演示要按 INA219 源码验证。

### 5.5 串口认证模块接法

用于接门锁板、RFID 板、LZ3663 或 USB-TTL 测试：

```text
外部认证板 GND -> RK2206 GND
外部认证板 TX  -> RK2206 B6 / GPIO0_PB6 / UART0_RX
外部认证板 RX  -> RK2206 B7 / GPIO0_PB7 / UART0_TX
外部认证板 5V  -> 独立 5V 或 RK2206 5V，注意共地
```

串口参数：

```text
115200 baud
8 data bits
no parity
1 stop bit
```

可以发送的文本命令：

| 文本 | RK2206 动作 |
|---|---|
| `AUTH_OK`, `CARD_OK`, `FP_OK`, `PASS_OK`, `OPEN`, `OK`, `1` | 授权成功，进入充电流程，K1 吸合 |
| `AUTH_DENY`, `DENY`, `FAIL`, `NO`, `0` | 授权失败 |
| `STOP`, `CUT_OFF`, `CUTOFF` | 远程断电，K1 断开，K2 散热 |
| `RESET` | 复位回待机 |

## 6. 安全边界

比赛阶段只做低压演示：

1. 不接 220V 市电。
2. 不让 RK2206 直接给小风扇、小电机等负载供电。
3. 继电器和负载优先使用独立 5V/12V 电源。
4. RK2206 与外部电源只共 GND，不互相反灌 5V。
5. 如果 3.3V GPIO 直连 5V 低电平继电器不稳定，加入 NPN 三极管或 ULN2003 做隔离驱动。
6. 所有高压、市电、真实电动车充电部分只写成产品设想，不在比赛台架上直接接入。

## 7. 软件开发环境和工具

| 类型 | 工具/环境 | 用途 |
|---|---|---|
| 编译环境 | Docker 容器 `txsmart-oh` | 在容器内执行 OpenHarmony `hb build` |
| 编译命令 | `hb build -f` | 生成 RK2206 固件 |
| 交叉工具链 | `gcc-arm-none-eabi-10.3-2021.10` | 编译 LiteOS / OpenHarmony 板端代码 |
| 烧录工具 | RKDevTool | 把 `Firmware.img` 烧录到 RK2206 |
| 串口工具 | MobaXterm 或其他串口助手 | 看启动日志、WiFi、MQTT、状态机日志 |
| 云端 | 华为云 IoTDA 或 MQTT Broker | 接收属性上报、下发控制命令 |
| 文档工具 | Markdown / Word / 截图 | 做计划书、阶段报告、接线图、答辩材料 |

当前编译命令模板：

```bash
docker exec txsmart-oh bash -lc 'export PATH=/home/tools/gcc-arm-none-eabi-10.3-2021.10/bin:$PATH; cd /home/openharmony && hb build -f'
```

固件输出路径：

```text
path/to/txsmartropenharmony/out/rk2206/isoftstone-rk2206/images/Firmware.img
```

云端参数写入位置：

```text
src/iot.c:
  HOST_ADDR
  CLIENT_ID
  DEVICE_ID
  MQTT_DEVICES_PWD

iot_smart_home_example.c:
  ROUTE_SSID
  ROUTE_PASSWORD
```

注意：总结文档和提示词里不要写真实 WiFi 密码、IoTDA 密钥、MQTT Password。另一个项目要用时让开发者现场填入。

## 8. 当前软件状态机

本项目状态机可以作为另一个项目的模板：

| 状态 | 含义 | 主要硬件动作 |
|---|---|---|
| `IDLE` | 待机 | K1 断、K2 断、绿灯/待机界面 |
| `CHARGING` | 正在充电 | K1 通，显示电压/电流/功率，板载电机可作为可视化反馈 |
| `FULL` | 充满 | K1 断，提示拔出或结束 |
| `OCCUPIED` | 占位 | K1 断，提示占位或异常停留 |
| `ALERT` | 风险告警 | K1 可保持或准备切断，K2 开，蜂鸣/RGB 告警 |
| `CUT_OFF` | 保护断电 | K1 断，K2 可继续散热，必须人工复位 |
| `FAULT` | 故障锁定 | K1 断，K2 保持，声光告警，等待维护 |

命令模板：

| 命令 | 来源 |
|---|---|
| `START` / `ACCESS_AUTH_OK` | 按键、串口认证、MQTT |
| `STOP` | 按键或云端 |
| `RESET` | 人工复位 |
| `RELAY_OFF` / `remote_cutoff` | 远程断电 |
| `SIM_GAS_WARN`, `SIM_TEMP_WARN`, `SIM_CURRENT_WARN`, `SIM_FULL` | 比赛演示用模拟事件 |

另一个项目可以保留“状态机 + 命令 + 事件 + 风险等级”这种写法，只替换状态名称和硬件动作。

## 9. MQTT / 华为云 IoTDA 思路

本项目的 MQTT 服务名是：

```text
safeCharge
```

主要上报属性：

| 属性 | 含义 |
|---|---|
| `deviceId` | 设备 ID |
| `chargeState` | 当前充电状态 |
| `riskLevel` | 风险等级 |
| `voltageV`, `currentA`, `powerW`, `energyWh` | 电压、电流、功率、能耗 |
| `relayStatus` | K1 继电器状态 |
| `coolingStatus` | K2 散热状态 |
| `alarmStatus` | 告警状态 |
| `lightStatus` | RGB 状态 |
| `networkStatus` | MQTT 在线状态 |
| `powerSensor` | `INA219` 或 `SIMULATED` |
| `eventCount`, `faultCode`, `eventTrace` | 事件统计、故障码、追溯信息 |

主要下发命令：

```text
safe_charge_control
```

常用动作：

```text
start
stop
reset
relay_off
remote_cutoff
demo_next
sim_gas_warn
sim_temp_warn
sim_current_warn
sim_full
```

另一个项目效仿时，只需要把 `safeCharge` 改成自己的服务名，把属性换成新项目的数据模型。

## 10. 推荐开发步骤

### 第 1 步：冻结作品主线

先写清楚：

1. 项目解决什么真实问题。
2. 比赛现场能看到什么硬件动作。
3. 哪些是端侧真实运行，哪些只是云端或展示。
4. 哪些硬件必须接，哪些后期扩展。

本项目的判断是：南向硬件闭环优先，北向平台辅助展示。

### 第 2 步：跑通开发环境

验收标准：

1. Docker 编译环境可用。
2. `hb build -f` 能生成 `Firmware.img`。
3. RKDevTool 能烧录。
4. 串口能看到系统启动日志。
5. LCD 能显示主界面。
6. WiFi/MQTT 初始化流程能进入。

### 第 3 步：先做软件闭环

在不接外部硬件之前，先跑通：

```text
按键/串口/MQTT 命令
-> 状态机切换
-> LCD 文案变化
-> RGB/蜂鸣/电机反馈
-> MQTT 上报状态
```

这样即使硬件没到，也能先做出可演示版本。

### 第 4 步：接 K1 单路继电器

只接一个低压负载：

```text
B2/GPIO0_PB2 -> IN1
COM1/NO1 -> 小灯或小风扇
```

验收标准：

1. 待机时负载断。
2. 授权/开始充电时负载通。
3. 断电保护时负载断。
4. LCD、串口、MQTT 的 `relayStatus` 和实物一致。

### 第 5 步：接 K2 散热继电器

```text
B1/GPIO0_PB1 -> IN2
COM2/NO2 -> 小风扇
```

验收标准：

1. 告警时 K2 开。
2. 保护断电时 K1 断、K2 仍可开。
3. 复位后 K1/K2 都断。

### 第 6 步：接功率检测

优先按当前源码接 INA219：

1. I2C 能读到 INA219。
2. LCD 显示真实电压、电流、功率。
3. MQTT 上报 `powerSensor = INA219`。
4. 断开 INA219 时仍能自动回到模拟数据，不影响演示。

### 第 7 步：接认证扩展

先用 USB-TTL 或串口助手模拟认证，再接门锁板、RFID 或 LZ3663：

```text
发送 AUTH_OK -> 充电开始
发送 STOP    -> 保护断电
发送 RESET   -> 回待机
```

### 第 8 步：整理比赛展示材料

必须准备：

1. 总体架构图。
2. 硬件接线图。
3. 状态机图。
4. MQTT 数据模型截图。
5. 低压断电演示视频或现场脚本。
6. 当前实现 vs 目标产品的差异表。

## 11. 另一个项目可直接照搬的提示词

可以把下面这段发给另一个 Codex 工作线程：

```text
我有一个嵌入式比赛项目，请参考“OpenHarmony RK2206 社区电动车安全充电终端”的开发方式来规划和落地。

要求：
1. 先梳理项目真实场景和比赛展示主线。
2. 南向硬件开发优先，云端/AI/后台只作为闭环展示。
3. 先做软件状态机，再接单路低压执行器，最后扩展多路硬件。
4. 给出硬件清单、接线方法、开发环境、编译烧录工具、源码模块分工、验收步骤。
5. 每一步都要写清楚当前要做什么、用到什么硬件、怎么验证。
6. 不要直接接 220V 或真实危险负载，先用 5V/12V 低压负载演示。
7. 如果工程里已有代码，要先盘点当前代码，再给出当前实现 vs 目标实现差异表。

可复用模板：
主控板 -> 传感器输入 -> 执行器输出 -> 本地状态机 -> LCD/灯光/声音反馈 -> MQTT/云端 -> 比赛展示文档。
```

## 12. 最重要的经验

1. 不要一上来接所有硬件。先让单路低压继电器闭环稳定。
2. 不要把云端当主角。比赛评分更看重端侧硬件控制、状态机和真实动作。
3. 不要让代码和接线脱节。接口号必须以当前源码为准。
4. 不要把真实高压接入比赛台架。低压负载足够证明控制逻辑。
5. 不要只写计划书。每个阶段都要有“接线、编译、烧录、串口、LCD、MQTT、实物动作”的验收项。
6. 另一个项目效仿时，保留方法论，替换场景、传感器、执行器和云端模型。
