# OpenHarmony社区电动车安全充电终端_当前开发状态检查报告

检查时间：2026-07-06 23:35
当前主线：RK2206 社区电动车安全充电终端
当前范围决定：智能门锁/门禁板先暂停，不再作为本轮主线功能。

## 1. 本次检查结论

当前项目已经形成了“南向 RK2206 硬件控制 + 北向 Web 管理页面 + 华为云 IoTDA 接入预留”的整体结构。

现在可以继续推进的主线是：

```text
RK2206 安全充电终端
-> 继电器 K1/K2/K3/K4
-> 电机模拟充电可视化
-> 温度/烟雾/人体/电流状态监测
-> LCD 本地状态展示
-> 华为云 IoTDA 数据上报/命令下发
-> 北向 Web 页面远程查看和控制
```

智能门锁/门禁板目前不再纳入主流程。它后续可以作为“身份认证拓展模块”单独展示，但不应该继续影响当前安全充电终端的烧录、按键、继电器、风扇和云端联调。

## 2. 当前固件状态

### 2.1 当前实际编译目标

当前 OpenHarmony 编译入口为：

```text
path/to/txsmartropenharmony/vendor/isoftstone/rk2206/samples/BUILD.gn
```

当前启用的是：

```text
./e1_iot_smart_home_hwiot:iot_smart_home_example
```

当前已经注释掉的是：

```text
./e3_door_auth_terminal:door_auth_terminal
```

这说明当前烧录到 RK2206 的主程序是安全充电终端，不是门禁板程序。

### 2.2 本次已经去掉的门禁干扰

已经从安全充电主固件中去掉：

```text
access_auth.c 编译项
access_auth.h 引用
access_auth_init() 初始化
event_access_auth 事件处理入口
smart_home_access_auth_process() 桥接处理
```

保留说明：

```text
CHARGE_CMD_ACCESS_AUTH_OK
access_authorized
```

这两个不是门禁板功能，而是安全充电流程里的“授权通过”状态。现在它可以由 RK2206 上键、本地演示、云端命令触发，不依赖智能门锁板。

## 3. RK2206 安全充电主流程

当前主流程设计如下：

```text
待机安全
-> 授权通过
-> K1 吸合，模拟开始充电
-> 电机转动，表示正在充电
-> 持续检测温度、电流、烟雾、人体存在
-> 发现风险，进入告警/断电保护
-> K1 断开，电机停止
-> K2 散热开启，K3 报警，K4 保护指示
-> 人工复位
-> 回到待机安全
```

当前 LCD 页面不是单纯图片，而是由状态机实时刷新：

```text
状态卡片
流程进度条
温度
电流
烟雾
输出通断
散热状态
人体检测
授权状态
事件计数
```

## 4. 当前按键逻辑

本次已把 ADC 按键处理改成统一事件通道，避免之前“有些键直接处理、有些键走事件队列”导致的延迟和不一致。

当前 RK2206 功能按键建议演示逻辑：

```text
上键 / KEY_UP
-> 授权通过
-> 开始充电
-> K1 通
-> 电机转

下键 / KEY_DOWN
-> 复位
-> 解除告警/断电状态
-> 回到待机安全

左键 / KEY_LEFT
-> 模拟烟雾/气体异常
-> 进入风险告警

右键 / KEY_RIGHT
-> 立即断电保护
-> K1 断
-> 电机停
-> K2/K3/K4 保护动作
```

当前仍需硬件实测的问题：

```text
如果按键仍然需要按几下才响应，需要继续调 ADC 电压阈值。
如果某个键误判，需要用串口日志看打印的 key 编号和电压值。
```

## 5. 继电器与执行器状态

当前南向硬件角色：

```text
K1：模拟充电输出继电器
K2：散热风扇继电器
K3：报警/蜂鸣/警示继电器
K4：保护/锁定/备用继电器
电机：板载电机，模拟“正在充电”的可视化状态
```

当前软件逻辑：

```text
充电中：
K1 通
电机转
K2 按温度/风险决定是否开启

断电保护：
K1 断
电机停
K2 开
K3 开
K4 开
LCD 显示断电/风险状态
```

如果风扇仍然不转，需要优先查硬件：

```text
1. K2 继电器有没有吸合
2. K2 COM 有没有接 5V 正极
3. K2 NO 有没有接风扇红线
4. 风扇黑线有没有接 GND
5. 风扇是否为 5V 风扇
6. 直接 5V + GND 给风扇时是否能转
```

## 6. 传感器状态

当前已纳入主固件的检测模块：

```text
温湿度：SHT30
光照：BH1750
可燃气体/烟雾：MQ2 ADC
人体感应：GPIO 人体传感器
电流/功率：当前代码仍是 INA219 逻辑
```

特别注意：

你买的是 INA226 电压电流模块，但当前 `power_sensor.c` 仍然是 INA219 驱动逻辑。
所以目前电流/功率很可能还是模拟数据，或者不能稳定读取真实电流。

下一步如果要做“真实电压、电流、功率检测”，需要把 `power_sensor.c` 从 INA219 改成 INA226：

```text
I2C 地址确认
配置寄存器
总线电压寄存器
分流电压寄存器
电流计算
功率计算
校准值
```

## 7. 华为云 IoTDA 状态

当前项目里存在两条 IoTDA 路线：

### 7.1 板端 MQTT

RK2206 固件的 `iot.c` 已经实现：

```text
MQTT 连接
safeCharge 服务属性上报
safe_charge_control 命令解析
K1/K2/K3/K4 状态上报
温度/电流/烟雾/风险等级上报
远程断电/复位/模拟告警等命令处理
```

注意：源码中存在云端连接信息，后续提交比赛公开材料前必须脱敏或改成配置项，不能直接公开密钥。

### 7.2 北向 Web 后端 REST

北向页面目录：

```text
web/northbound-safe-charge-dashboard
```

当前已实现：

```text
GET  /api/device/state
POST /api/device/command
safe_charge_control 命令封装
remote_cutoff
reset_alarm
toggle_cooling
set_thresholds
```

当前问题：

```text
northbound-safe-charge-dashboard\.env 不存在
```

因此北向页面现在默认不能真正访问华为云 IoTDA REST，只能走本地后端模拟闭环。
要让页面真正连云，需要按 `.env.example` 创建 `.env`，填入华为云项目 ID、AK/SK、设备 ID、服务 ID 等信息。

## 8. 前端页面状态

前端项目已经重新构建通过：

```text
npm run build
vite v6.4.2
6181 modules transformed
dist/index.html
dist/assets/index-lSdNyS60.css
dist/assets/index-D8w3Vp7f.js
```

前端页面当前具备：

```text
多端口充电状态展示
风险分展示
事件时间线
K1/K2 继电器状态展示
远程断电确认
复位告警
散热开关
阈值设置
IoTDA/本地模式提示
```

当前不足：

```text
还没有真实 IoTDA REST 运行时配置
页面命令只覆盖远程断电、复位、散热、阈值
还没有把板端 MQTT 实时数据直接打通到页面实时状态
```

## 9. Docker/OpenHarmony 编译状态

本次已经在 Docker 容器中完成 OpenHarmony 编译。

之前 Docker 内 `ohos_config.json` 指向旧路径：

```text
/mnt/d/openharmony/txsmartropenharmony
```

已修正为：

```text
/home/openharmony
```

编译命令完成结果：

```text
isoftstone-rk2206 build success
cost time: 0:02:19
```

新的烧录产物位置：

```text
path/to/txsmartropenharmony/out/rk2206/isoftstone-rk2206/images/Firmware.img
path/to/txsmartropenharmony/out/rk2206/isoftstone-rk2206/images/rk2206_db_loader.bin
```

已经复制到比赛工作区根目录：

```text
local-build/Firmware.img
local-build/rk2206_db_loader.bin
```

复制前的旧烧录文件已备份：

```text
local-build/firmware_backups/build_20260706_233109
```

## 10. 当前不要做的事情

为了不把项目重新弄乱，当前阶段先不要做：

```text
不要继续把智能门锁板接入主流程
不要继续把指纹/密码/NFC 作为安全充电必须条件
不要再烧门禁板程序到 RK2206 主控
不要把门禁屏幕当安全充电主屏
不要继续用静态图片冒充真实交互页面
```

门禁板后续可以作为第二阶段拓展：

```text
用户身份认证扩展
授权后通过串口/星闪/云端给 RK2206 发 AUTH_OK
但现在先不做
```

## 11. 下一步执行顺序

### 第一步：只烧 RK2206 安全充电主板

烧录文件选：

```text
local-build/Firmware.img
local-build/rk2206_db_loader.bin
```

烧录对象：

```text
RK2206 安全充电主板
```

不要选门禁板。

### 第二步：串口确认启动日志

烧录后打开串口，重点看：

```text
[safe_charge] app init
[safe_charge] smart_home_thread boot
[safe_charge] gas sensor init done
[safe_charge] human sensor init done
[safe_charge] charge_safety_init done
```

如果 WiFi 和 IoTDA 正常，还应看到 MQTT 连接和属性上报日志。

### 第三步：验证按键

按键测试顺序：

```text
上键：开始充电，K1 通，电机转
右键：断电保护，K1 断，电机停，K2/K3/K4 开
下键：复位，回到待机
左键：模拟烟雾/气体风险
```

如果按键仍然慢：

```text
先记录串口输出的 key 编号和电压值
再调 adc_key.c 的阈值
```

### 第四步：验证 K2 风扇

断电保护时软件会让 K2 开。
如果风扇不转，先不要改代码，先测：

```text
K2 是否吸合
K2 COM 是否有 5V
K2 NO 吸合后是否输出 5V
风扇黑线是否接 GND
风扇直接接 5V/GND 是否能转
```

### 第五步：配置北向页面 IoTDA

创建：

```text
web/northbound-safe-charge-dashboard/.env
```

参考：

```text
web/northbound-safe-charge-dashboard/.env.example
```

配置完成后再运行：

```text
npm run dev
```

然后测试页面远程断电、复位、散热、阈值设置是否能真正下发到 IoTDA。

### 第六步：把 INA226 接入真实电流检测

当前真实电流检测还不是最终状态。
下一轮应该单独改 `power_sensor.c`，把 INA219 改成 INA226，然后再测试：

```text
车端电压
充电桩输出电压
充电电流
功率
满电低电流判断
过流保护
```

## 12. 当前项目一句话状态

当前项目已经从“门禁/充电混在一起”重新整理为：

```text
RK2206 安全充电终端为主线，
门禁认证暂时移除，
继电器、电机、传感器、LCD、IoTDA、北向页面进入可继续联调状态。
```
