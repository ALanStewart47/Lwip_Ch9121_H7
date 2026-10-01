# CST-MDPS24120C-4TDP

基于 STM32G431RBTx 的四通道灯控固件工程。工程包含亮度/PWM 控制、触发与可编程模式、按键显示、串口通信，以及 W5500 网络通信相关代码。

## 快速启动

1. 使用 Keil MDK 打开 `MDK-ARM/STM32G431.uvprojx`。
2. 选择工程中的 `STM32G431` target 后执行 Build。
3. 使用已确认适配本硬件的下载器和烧录流程写入目标板；本仓库不把未验证的烧录参数当作通用默认值。
4. 修改前先阅读本文件、`STATUS.md` 和与任务相关的源码；发布相关改动还需阅读 `CHANGELOG.md`。

## 主要目录

- `App/`：应用功能，包括灯控、触发、参数存储、串口和主从控制。
- `Bsp/`：板级驱动与外设抽象。
- `Core/`：CubeMX 生成的初始化、启动和中断代码；主入口为 `Core/Src/main.c`。
- `Protocol/`：通信协议解析与封包。
- `MDK-ARM/`：Keil 工程；实际编译文件以 `STM32G431.uvprojx` 的 target 清单为准。
- `Drivers/`：STM32 HAL/CMSIS 等第三方驱动。
- `_Doc/`：历史审查记录和工作笔记；其中内容需要结合当前代码复核，不能直接视为发布结论。

## 关键入口与约束

- `Core/Src/main.c`：完成 GPIO、DMA、ADC2、USART2、SPI2、SPI3、TIM3/TIM6/TIM7、IWDG 初始化，再进入 `bsp_Init()` 和主循环。
- `App/inc/app_light.h`：当前固件对外报告的主/次版本宏 `MAJOR_VERSION` 与 `MINOR_VERSION`。
- 当前 Keil target 为 `STM32G431`，芯片配置为 `STM32G431RBTx`。
- 源码中存在未编入当前 Keil target 的文件。修改或评审功能时，应先确认该文件是否实际参与构建，避免把样例或停用路径当成在产代码。

## 版本与发布

版本真相以“代码内版本宏 + Git tag”为准，不按复制出的版本文件夹管理。

- 发行 tag 格式：`cst-mdps24120c-v<主>.<次>.<补丁>`，例如 `cst-mdps24120c-v1.0.1`。
- 每次正式发布前，更新代码内版本、`CHANGELOG.md`，完成目标硬件回归验证后再创建 tag。
- 发现已发布版本的缺陷时，从对应 tag 建立维护分支，修复和验证后发布新的补丁 tag；同一修复应合入后续开发线。

## 文档入口

- [`STATUS.md`](STATUS.md)：当前进度、验证状态、风险与下一步。
- [`CHANGELOG.md`](CHANGELOG.md)：正式发行版的变更记录。

