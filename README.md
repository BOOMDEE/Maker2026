# 🚀 2026 STEM 展评：应急物资速递系统 (ESP32-S3)

**核心目标：** 在 1.2m 模拟赛道中，承载 200g 物资（鸡蛋）安全通过 15° 斜坡并完成多路避障。

---

## 🛠 1. 硬件架构与接线定义

### A. 动力系统 (Motor Control)
使用 ESP32-S3 的 **GPIO 4-11**，采用 PWM 调速。

| 电机位置 | 引脚 A | 引脚 B | PWM 通道 | 物理接线参考 |
| :--- | :--- | :--- | :--- | :--- |
| **前左 (FL)** | GPIO 4 | GPIO 5 | CH 0 / 1 | 对应 L298N/TB6612 左前通道 |
| **后左 (RL)** | GPIO 6 | GPIO 7 | CH 2 / 3 | 对应驱动板左后通道 |
| **前右 (FR)** | GPIO 8 | GPIO 9 | CH 4 / 5 | 对应驱动板右前通道 |
| **后右 (RR)** | GPIO 10 | GPIO 11 | CH 6 / 7 | 对应驱动板右后通道 |

### B. 感知矩阵 (Sensor Matrix)
四路超声波实现 360° 环境感知。

| 位置 | Trig (触发) | Echo (回响) | 作用描述 |
| :--- | :--- | :--- | :--- |
| **前 (dist1)** | **16** | **15** | 核心避障，探测前方障碍 |
| **左侧 (dist2)** | **21** | **38** | 贴墙巡航，修正航向防止擦碰 |
| **右侧 (dist3)** | **39** | **40** | 贴墙巡航，修正航向防止擦碰 |
| **姿态 (IMU)** | **SCL: 1** | **SDA: 2** | 使用 **LIS2DW12** 检测坡度 |

---

## 💻 2. PlatformIO 环境配置 (`platformio.ini`)

针对 **N16R8** 型号的满血配置：

```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_opi ; 开启 8MB PSRAM
monitor_speed = 115200

build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1 ; 强制开启串口输出
    -D BOARD_HAS_PSRAM

lib_deps =
    adafruit/Adafruit LIS2DW12 @ ^1.1.2
    adafruit/Adafruit BusIO
    Wire
```

---

## 🧠 3. 核心控制逻辑简述

系统采用**状态机 (State Machine)** 架构，优先级如下：

1.  **倾角保护 (Slope Mode)**：检测到 $Pitch < -2.5$ 时进入“电子点刹”，利用 PWM 占空比循环切换实现重载下坡防冲撞。
2.  **紧急避障 (Obstacle Mode)**：当前方传感器距离 $< 20\text{cm}$ 时，执行“原地坦克转向”。
3.  **航向修正 (Cruise Mode)**：当侧向距离 $< 15\text{cm}$ 时，微调左右轮差速，确保在 1.2m 赛道中央行驶。

---

## ⚠️ 4. 安全操作指南

* **架空测试**：在上传任何涉及 `ledcWrite` 的代码前，必须使用 **垫高块** 将小车架空。
* **电压隔离**：
    * **ESP32-S3** 通过 USB 或稳压模块供电。
    * **共地**：所有 GND 必须互连。
* **串口排烟**：若监视器无输出，请按住 `BOOT` 键重新插拔 USB 并点击 `Upload`。
* **逐步调试**：建议先实现单轮控制，再逐步增加避障和坡度保护功能，确保每一步都在安全环境下验证。