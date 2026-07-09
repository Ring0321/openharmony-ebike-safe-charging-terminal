# OpenHarmony 社区电动车安全充电终端 - 华为云 IoTDA 配置清单

本文用于当前项目先做“华为云端”这一阶段。目标不是先改板端逻辑，而是先把华为云 IoTDA 的产品、模型、设备、命令和板端代码对齐。云端建好后，再把控制台生成的连接参数写入板端 `iot.c`，重新编译固件。

## 1. 当前板端已经具备什么

当前工程路径：

```text
path/to/txsmartropenharmony/vendor/isoftstone/rk2206/samples/e1_iot_smart_home_hwiot
```

当前板端已经按华为云 IoTDA 预置 Topic 写了 MQTT 逻辑：

```text
属性上报 Topic:
$oc/devices/{device_id}/sys/properties/report

命令订阅 Topic:
$oc/devices/{device_id}/sys/commands/#

命令响应 Topic:
$oc/devices/{device_id}/sys/commands/response/request_id={request_id}
```

当前上报服务名固定为：

```text
safeCharge
```

注意区分两个 ID：

```text
IoTDA 设备 ID:
用于 MQTT 登录、Topic 中的 {device_id}，由华为云注册设备后生成。

业务 deviceId:
上报属性里的 deviceId，目前是 rk2206_charge_01，用来给评委看“这是 1 号充电终端”。
```

## 2. 华为云控制台第一步

进入：

```text
华为云控制台 -> IoT 物联网 -> 设备接入 IoTDA
```

建议优先选择和当前代码一致的区域：

```text
华北-北京四 cn-north-4
```

如果你选了其他区域，也可以，但后面 `HOST_ADDR` 一定要换成该区域控制台给出的接入地址。

## 3. 创建产品

在 IoTDA 控制台中创建产品：

| 项目 | 建议填写 |
|---|---|
| 产品名称 | SafeChargeTerminal |
| 协议类型 | MQTT |
| 数据格式 | JSON |
| 设备类型 | 充电桩 / 自定义设备 / 网关外设均可，优先自定义设备 |
| 厂商名称 | OpenHarmony-RK2206 |
| 产品描述 | 社区电动车安全充电终端，支持安全充电、风险告警、断电保护、散热联动和云端远程控制 |

创建后进入产品的“模型定义 / 产品模型”。

## 4. 创建服务 safeCharge

新增服务：

| 项目 | 填写 |
|---|---|
| 服务 ID | safeCharge |
| 服务类型 | 建议选择自定义 |
| 服务描述 | 安全充电终端状态、风险、功率、告警和控制信息 |

服务 ID 必须严格写成 `safeCharge`，大小写不要变。

## 5. safeCharge 属性模型

在 `safeCharge` 服务下新增以下属性。属性名必须和板端代码一致。

| 属性名 | 类型 | 单位/枚举建议 | 说明 |
|---|---|---|---|
| deviceId | string | - | 业务设备 ID，当前为 rk2206_charge_01 |
| portId | string | - | 充电口 ID，当前为 port_1 |
| userId | string | - | 用户 ID，当前演示为 guest |
| sessionId | string | - | 当前充电会话 ID |
| chargeState | string | IDLE/CHARGING/FULL/OCCUPIED/ALERT/CUT_OFF/FAULT | 充电状态 |
| riskLevel | string | L0/L1/L2/L3/L4 或 SAFE/WARN/DANGER | 风险等级，按代码实际字符串显示 |
| riskScore | int | 0-100 | 风险评分 |
| lastEvent | string | - | 最近一次事件 |
| eventTrace | string | - | 事件追踪文本 |
| voltageV | decimal | V | 电压 |
| currentA | decimal | A | 电流 |
| powerW | decimal | W | 功率 |
| energyWh | decimal | Wh | 电量累计 |
| costEstimate | decimal | 元 | 费用估算 |
| temperatureC | decimal | C | 板载 SHT30 附近环境温度 |
| humidityPct | decimal | % | 湿度 |
| smokeLevel | decimal | ppm/level | MQ2 可燃气体/烟雾值 |
| humanPresence | string | PRESENT/NONE | 人体感应状态 |
| accessAuthorized | string | YES/NO | 是否授权通过 |
| relayStatus | string | ON/OFF | K1 充电输出继电器 |
| chargeMotorStatus | string | ON/OFF 或自定义文本 | 板载电机状态 |
| alarmStatus | string | ON/OFF | 声光/蜂鸣告警状态 |
| coolingStatus | string | ON/OFF | K2 散热风扇状态 |
| lightStatus | string | RED/GREEN/BLUE/OFF 等 | RGB 状态灯 |
| demoStep | int | 0-99 | 演示步骤编号 |
| networkStatus | string | ONLINE/OFFLINE | 板端判断的 MQTT 联网状态 |
| powerSensor | string | INA219/SIMULATED/OFFLINE | 功率传感器来源 |
| eventCount | long | - | 事件计数 |
| faultCode | long | - | 故障码 |
| temperatureLimit | decimal | C | 温度告警阈值 |
| currentLimit | decimal | A | 电流告警阈值 |
| smokeLimit | decimal | ppm/level | 烟雾/气体告警阈值 |

访问方式建议先全部选：

```text
R
```

也就是只读上报。阈值类属性以后如果改成设备影子/属性设置，再单独改为 RW。

## 6. 新增命令 safe_charge_control

在 `safeCharge` 服务下新增命令：

```text
safe_charge_control
```

命令参数：

| 参数名 | 类型 | 是否必填 | 说明 |
|---|---|---|---|
| action | string | 是 | 控制动作 |
| temperatureLimit | decimal | 否 | 修改温度告警阈值时使用 |
| temperatureCutoff | decimal | 否 | 修改温度断电阈值时使用 |
| currentLimit | decimal | 否 | 修改电流告警阈值时使用 |
| currentCutoff | decimal | 否 | 修改电流断电阈值时使用 |
| smokeLimit | decimal | 否 | 修改烟雾/气体告警阈值时使用 |
| smokeCutoff | decimal | 否 | 修改烟雾/气体断电阈值时使用 |
| fullCurrent | decimal | 否 | 充满判断电流阈值 |

`action` 当前支持：

| action | 板端效果 |
|---|---|
| start_charge | 授权并开始充电，K1 输出打开 |
| authorize | 等价于授权通过 |
| access_grant | 等价于授权通过 |
| auth_deny | 授权拒绝 |
| stop_charge | 停止充电 |
| reset_alarm | 复位告警/保护状态 |
| remote_cutoff | 远程断电保护，K1 立即断开 |
| relay_off | 等价于远程断电保护 |
| simulate_smoke | 模拟烟雾异常 |
| clear_smoke | 清除烟雾模拟 |
| simulate_full | 模拟充满 |
| simulate_temp_warn | 模拟温度告警 |
| simulate_current_warn | 模拟电流告警 |
| simulate_gas_warn | 模拟可燃气体告警 |
| demo_next | 演示步骤切换 |
| set_thresholds | 修改阈值 |

## 7. 命令下发测试 JSON

开始充电：

```json
{
  "command_name": "safe_charge_control",
  "paras": {
    "action": "start_charge"
  }
}
```

远程断电保护：

```json
{
  "command_name": "safe_charge_control",
  "paras": {
    "action": "remote_cutoff"
  }
}
```

复位：

```json
{
  "command_name": "safe_charge_control",
  "paras": {
    "action": "reset_alarm"
  }
}
```

模拟可燃气体告警：

```json
{
  "command_name": "safe_charge_control",
  "paras": {
    "action": "simulate_gas_warn"
  }
}
```

修改阈值：

```json
{
  "command_name": "safe_charge_control",
  "paras": {
    "action": "set_thresholds",
    "temperatureLimit": 42,
    "temperatureCutoff": 52,
    "currentLimit": 0.7,
    "currentCutoff": 1.0,
    "smokeLimit": 30,
    "smokeCutoff": 80,
    "fullCurrent": 0.08
  }
}
```

## 8. 注册设备

在 IoTDA 中进入：

```text
设备 -> 所有设备 -> 注册设备
```

建议填写：

| 项目 | 建议 |
|---|---|
| 所属产品 | SafeChargeTerminal |
| 设备标识码 nodeId | rk2206_charge_01 |
| 设备认证类型 | 密钥 |
| 密钥 | 可以自定义，也可以让平台生成 |

注册成功后必须保存：

```text
设备 ID device_id
设备密钥 secret
MQTT 连接参数中的 ClientId
MQTT 连接参数中的 Username
MQTT 连接参数中的 Password
平台接入地址 Broker Address
```

注意：板端代码里要写的是 MQTT 连接参数中的 `Password`，不是直接把原始设备密钥随便填进去。华为云文档说明，Password 通常由设备密钥和时间戳通过 HMACSHA256 生成；控制台的“MQTT 连接参数”或官方参数生成工具会直接给出可用值。

## 9. 板端代码后续需要替换的位置

文件：

```text
path/to/txsmartropenharmony/vendor/isoftstone/rk2206/samples/e1_iot_smart_home_hwiot/src/iot.c
```

需要替换：

```c
#define HOST_ADDR "你的 IoTDA 接入地址"
#define CLIENT_ID "你的 MQTT ClientId"
#define DEVICE_ID  "你的 IoTDA 设备 ID"
#define MQTT_DEVICES_PWD "你的 MQTT Password"
```

WiFi 热点在：

```text
path/to/txsmartropenharmony/vendor/isoftstone/rk2206/samples/e1_iot_smart_home_hwiot/iot_smart_home_example.c
```

需要确认：

```c
#define ROUTE_SSID      "你的 2.4GHz WiFi 或手机热点"
#define ROUTE_PASSWORD  "你的 WiFi 密码"
```

当前板端使用 MQTT 1883 非加密端口。如果你在云端选择 MQTTS 8883，需要额外加 CA 证书和 TLS 连接代码；本阶段为了先跑通比赛演示，先用 MQTT 1883。

## 10. 云端验收顺序

1. 云端产品和 `safeCharge` 模型创建完成。
2. 云端设备注册完成，拿到连接参数。
3. 我把连接参数写入 `iot.c`，把 WiFi 写入 `iot_smart_home_example.c`。
4. Docker 重新 `hb build -f`。
5. 烧录新的 `Firmware.img`。
6. 串口看到 WiFi 连接成功、MQTT connect 成功。
7. IoTDA 控制台看到设备在线。
8. IoTDA 设备影子/最新属性里能看到 `chargeState`、`relayStatus`、`coolingStatus`、`humanPresence` 等属性变化。
9. 从控制台下发 `remote_cutoff`，板端 K1 断开，屏幕进入断电保护。

## 11. 参考资料

- 华为云 IoTDA 入门：创建 MQTT 产品、开发产品模型、注册设备、上报数据、下发命令。
- 华为云 IoTDA Topic 定义：属性上报、命令下发和命令响应均使用 `$oc/devices/{device_id}/sys/...` 预置 Topic。
- 华为云 IoTDA 密钥鉴权：设备注册后使用 DeviceId、ClientId、Username、Password 连接，Password 可由设备密钥和时间戳生成。
