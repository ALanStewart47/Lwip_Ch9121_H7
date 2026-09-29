# 项目交接文档

最后更新：2026-09-29（Asia/Shanghai）  
当前阶段：已修复初始化停顿和 UDP 端口字节序错误；修正版已下载，板上初始化、DHCP 获址和 ping 通过，CH9121 广播收发与官方工具验收仍待确认。  
当前授权：按用户确认的计划修改固件、Keil/CubeMX 配置和本交接文档。

## 1. 当前目标与范围

在正点原子 Apollo STM32H743 开发板上，使现有 lwIP 工程兼容 CH9121 的 UDP 广播配置协议，至少包括查询与修改。

用户已确认：H743 作为 CH9121 设备端，使用 RAW UDP 与 `NetModuleConfig_V2.04` 互操作；本阶段为裸机验证。仅可修改设备名、DHCP、IPv4、掩码和网关，参数仅保存在 RAM。串口透传、Flash 保存、恢复出厂命令和 RTOS 不在本阶段范围内。

设备默认名为 `H743_CH9121`，当前验证固件默认启用 DHCP；静态备份保留为 `192.168.1.30/24`、网关 `192.168.1.1`，关闭 DHCP 时使用。发现报文版本为 `0x47`，完整配置硬件/软件版本分别为 `0x04/0x07`。SET 校验通过时先通过旧网络发送 ACK，再由主循环应用；对范围外字段被修改的请求返回 `C1` 且不应用任何字段。

## 2. 用户协作与交接要求

- 文件名是 `hanoff.md`，沿用用户原始拼写。
- 用户要求时更新；重要开发、设计或验证节点自动更新。
- 用户说“项目暂时完成”等暂停或结束话术时，本轮结束前更新，记录完成项、未完成项、验证状态和恢复入口。
- 根目录 `AGENTS.md` 已保存这些维护约定，后续协作者开始工作时先读本文件。
- 当前无后台监控或定时任务；自动更新指项目工作过程中的节点维护。

## 3. 仓库与工程结构

| 路径 | 用途与现状 |
| --- | --- |
| `02_Software/Demo_Lwip/Demo_Lwip.ioc` | STM32CubeMX 配置，目标 STM32H743IITx/LQFP176，STM32Cube FW_H7 V1.13.0，RMII |
| `02_Software/Demo_Lwip/Core` | 应用入口、时钟、MPU、GPIO、中断；已接入 PHY 复位、网络服务与主循环，SystemInit 提前启用 D2 SRAM 时钟 |
| `02_Software/Demo_Lwip/LWIP/App` | `lwip.c` 负责网口初始化、收包/定时器/链路轮询；`ch9121_service.c/.h` 提供 CH9121 设备端配置服务和诊断接口 |
| `02_Software/Demo_Lwip/LWIP/Target` | ETH HAL 适配、零拷贝接收池、lwIP 选项 |
| `02_Software/Demo_Lwip/Middlewares/Third_Party/LwIP` | 当前 lwIP 2.2.1；阅读了相关 UDP、IPv4、内存实现与默认选项 |
| `02_Software/Demo_Lwip/Drivers/BSP/Components/lan8742` | 当前接入的 PHY 驱动 |
| `02_Software/Demo_Lwip/MDK-ARM/Demo_Lwip.uvprojx` | Keil 工程，ARMCC 5.06 update 6，已有输出目录 |
| `01_Reference/实验61 网络通信实验` | 正点原子网络例程：lwIP 1.4.1、LAN8720、PCF8574、软件 I²C、TCP/UDP/HTTP 演示 |
| `01_Reference/Apollo STM32F4&F7&H7_MotherBoard_V1.7.pdf` | 底板原理图，已核对以太网、核心板接口、IO 扩展及 PHY 复位电路 |
| `01_Reference/Apollo STM32H743_CORE_V1.0.pdf` | 核心板原理图，已核对 MCU、晶振及相关引脚 |
| `03` | 当前目录为空，未发现业务实现 |

初次检查时，Git 的 `main` 分支尚无提交，`01_Reference/` 与 `02_Software/` 均为未跟踪目录。未创建提交或进行版本控制清理。

2026-09-29 代码构成核对：当前为 CubeMX/ST 基础工程上新增的裸机配置服务，由 H743 自身的 ETH MAC 和板载 LAN8720A 提供网络连接。`Drivers/CMSIS` 提供处理器与寄存器定义，`Drivers/STM32H7xx_HAL_Driver` 提供外设 HAL，`Drivers/BSP/Components/lan8742` 提供复用的 PHY 基础驱动；本项目新增 `Drivers/BSP/Board/board_phy.c/.h` 和 `LWIP/App/ch9121_service.c/.h`，并修改生成的启动、入口与 ETH 适配代码。底层 lwIP 库包含通用网络协议，当前应用开放的是 CH9121 UDP 配置业务。

阅读入口与职责：

| 文件 | 主要职责 |
| --- | --- |
| `Core/Src/main.c` | MPU/DCache、HAL/系统时钟、GPIO、PHY 复位、lwIP/服务初始化；裸机主循环调用两个 process 函数 |
| `Core/Src/system_stm32h7xx.c` | `SystemInit()` 提前启用 D2 SRAM；在 C 运行库和 main 之前执行 |
| `Core/Src/stm32h7xx_it.c` | SysTick 更新 HAL tick、ETH 中断交给 HAL；应用协议处理仍由主循环执行 |
| `Drivers/BSP/Board/board_phy.c` | PH4/PH5 开漏软件 I²C，PCF8574 P7 PHY 复位、ACK 和错误状态 |
| `LWIP/App/lwip.c` | 创建 `gnetif`，每轮有界收包、lwIP 超时处理、100 ms PHY 链路轮询 |
| `LWIP/Target/ethernetif.c` | MAC/ETH/PHY 初始化、UID 派生 MAC、DMA/pbuf 接收池、发送和缓存处理、错误与断言记录 |
| `LWIP/Target/lwipopts.h` | 裸机/RAW API、DHCP、heap/对齐/校验和及 UDP 配置端口接收策略 |
| `LWIP/App/ch9121_service.c` | 搜索/GET/SET、固定偏移编解码、五项参数校验、应答、RAM 当前/待生效配置、DHCP/静态切换 |
| `MDK-ARM/Demo_Lwip/Demo_Lwip.sct` | Flash/RAM 与 ETH 描述符/RX 池的 Keil 链接布局，预留 heap 及管理空间 |
| `02_Software/tools/ch9121_udp_probe.py` | PC 端扫描、查询、写入读回、错误报文与名称边界验证 |

`ch9121_service.c` 的解析、校验、应答和网络参数应用通过函数分工，仍处于同一个源文件：`receive_callback()` 接收并分派命令，`encode_config()` 生成完整配置，`decode_candidate()` 整体校验候选配置，`send_reply()` 广播应答，`ch9121_service_process()` 在 ACK 成功后应用参数。`active_config` 保留当前配置与静态地址备份；`pending_config/apply_pending` 表示等待主循环生效的写入。参数仅存 RAM，两个串口透传端口为关闭占位；固定 19200/8N1 不表示已经实现串口通信。

本次仅核对当前源码并补充结构交接；没有修改固件、重新构建或新增实机验证。最近直接实机结果仍为 2026-09-28 的初始化/DHCP/ping 记录，CH9121 官方工具互操作仍未验收。

## 4. 硬件核对结果

资料所示硬件为 Apollo 底板 V1.7 + STM32H743IIT6 核心板 V1.0；用户实际板卡是否完全同修订，仍应在上板时核实。

- 以太网 PHY：底板 U1 为 **LAN8720A**，PHY 外部晶振为 25 MHz，RMII_REF_CLK 接 PHY 的 nINT/REFCLKO。
- MCU HSE：核心板 Y2 为 **25 MHz**，与当前 `HSE_VALUE=25000000` 一致。
- 当前软件 RMII 引脚与底板资料、参考例程一致：

| 信号 | STM32 引脚 |
| --- | --- |
| REF_CLK | PA1 |
| MDIO | PA2 |
| CRS_DV | PA7 |
| MDC | PC1 |
| RXD0 / RXD1 | PC4 / PC5 |
| TX_EN | PB11 |
| TXD0 / TXD1 | PG13 / PG14 |

- PHY 复位由 **PCF8574T 的 P7** 控制，通过 S8050 反相后连接 LAN8720A nRST。例程写 P7=1 拉低 nRST，延时后写 P7=0 释放复位。
- PCF8574 的 7 位 I²C 地址为 **0x20**；例程中 `PCF8574_ADDR=0x40` 是左移一位的写地址，不能混用。
- I²C 信号为 **PH4/SCL、PH5/SDA**；参考代码使用软件 I²C。
- 已新增 `Drivers/BSP/Board/board_phy.c/.h`：PH4/PH5 开漏软件 I²C、7 位地址 `0x20`、P7 输出影子值、地址/数据 ACK 检查；ETH 初始化前执行 P7 高/低各 100 ms，不关闭 SysTick 中断。静态代码路径已接入，电气时序和 ACK 尚未上板测量。
- 当前 PLL 设置为 480 MHz CPU / 240 MHz AHB；参考例程为 400 MHz / 200 MHz。需在实机核实芯片修订与当前时钟设置，而不是直接照搬旧例程。

## 5. 当前网络代码行为

### 5.1 调用链

`main()` 在 HAL/系统时钟和 GPIO 初始化后先调用 `Board_PHY_Reset()`，再初始化 lwIP 与 CH9121 UDP 服务。主循环持续调用 `MX_LWIP_Process()` 和 `ch9121_service_process()`；前者处理有界收包、lwIP 定时器与 100 ms PHY 链路轮询，后者应用已 ACK 的配置。

`MX_LWIP_Init()` 将接口先以 0 地址加入 lwIP，检查 `netif_add()` 结果并启用网卡；CH9121 服务保持接口为 0 地址并在启动时开启 DHCP。静态备份为 `192.168.1.30/24`、网关 `192.168.1.1`，只有收到关闭 DHCP 的有效 SET 后才应用到接口。

### 5.2 lwIP 配置

- `NO_SYS=1`、`WITH_RTOS=0`：裸机。
- `LWIP_NETCONN=0`、`LWIP_SOCKET=0`：目前适合使用 RAW UDP API。
- `LWIP_DHCP=1`，当前验证固件上电默认启用 DHCP；静态备份 `192.168.1.30/24`、网关 `192.168.1.1` 保留在 RAM，收到关闭 DHCP 的有效 SET 后应用。
- `LWIP_UDP` 未覆盖，库默认值为 1，UDP 栈已经启用。
- 网卡设置了 `NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP`。
- `IP_SOF_BROADCAST` / `IP_SOF_BROADCAST_RECV` 沿用库默认 0，表示未启用按 PCB 的广播权限过滤，**不能据此说广播被关闭**。
- 硬件校验和开启，IP/UDP/TCP 软件校验关闭。
- `MEM_ALIGNMENT=32`；lwIP heap 固定于 `0x30004000`，大小 16 KiB。`LWIP_IP_ACCEPT_UDP_PORT` 允许 UDP 50000 在 DHCP 未获租约时仍接收配置广播。
- 协议处理应注意链式 pbuf，不能假定 UDP 完整载荷都在第一段，也不能按字符串处理二进制协议。
- lwIP IPv4 接收会检查网卡地址；已通过 `LWIP_IP_ACCEPT_UDP_PORT` 为 UDP 50000 开放 DHCP 未获租约时的配置广播接收路径，仍需上板抓包验证。

### 5.3 以太网与内存

- PHY 仍复用现有 LAN8742 驱动的基础寄存器及链路判断；驱动初始化前检查地址 0 的 PHYID1=`0x0007`、PHYID2 高 12 位=`0xC0F`，并要求驱动最终使用地址 0。驱动中的其余兼容性仍须实机验证。
- 稳定 MAC 由 STM32 UID 派生为本地管理单播地址，供 ETH 和 CH9121 身份字段共用。
- Keil map 已确认 RX 描述符 `0x30000000`、TX 描述符 `0x30000080`；描述符区大小 `0xE0`，D2 非缓存 RX 池从 `0x30000100` 开始，大小 `0x3120`，结束于 `0x30003220`。8 个缓冲区、32 字节对齐。
- lwIP heap 从 `0x30004000` 占用 16 KiB；scatter 在 `0x30009000` 才安排其余 D2 数据区，前述 RX 池、描述符和 heap 没有重叠。MPU 将 D2 SRAM 设为非缓存区，并用更高优先级区域为描述符设置 Device 属性。
- `Core/Src/system_stm32h7xx.c` 已启用 `DATA_IN_D2_SRAM`，在 `SystemInit()` 中打开 D2 SRAM1/2/3 时钟，保证启动运行库初始化和 lwIP 首次使用 heap 前可访问这些区域。先前工程缺少这一步；不能仅凭修复后运行成功断定原始断言内容。
- CubeMX 再生成注意：当前 D2 SRAM 时钟由上述启动代码宏控制，`.ioc` 未保存这个宏，Keil C/C++ 预定义中也尚未配置它。该行在 `system_stm32h7xx.c` 的非 USER CODE 区，不能仅依赖 `KeepUserCode=true` 保留；再生成后检查宏及 `SystemInit()` 启用路径。启动汇编先调用 `SystemInit()`，再进入 C 运行库 `__main`，因此不能把首次启用时钟简单推迟到 `main()`。本次仅核对和补充交接，没有重新生成代码或改变固件。
- 后续 CubeMX 新工程操作：生成 H743/Keil 工程后，在 Keil `Options for Target → C/C++ → Preprocessor Symbols → Define` 保留原有宏并追加 `DATA_IN_D2_SRAM`，完整重建；确认生成的 `system_stm32h7xx.c` 中仍有对应的 `SystemInit()` 时钟启用代码。若采用编译宏，源文件中的同名 `#define` 保持默认注释状态，避免重复定义；重新生成后再检查 Keil 宏设置。CubeMX 的 User Constants 默认写入 `main.h`，而当前 system 文件不包含该头文件，所以仅在 User Constants 中增加此宏无法启用此启动路径。该操作是给用户的新工程步骤，尚未迁移当前工程的宏位置。
- 官方依据已核对：ST 员工发布的 [STM32H7 Ethernet/lwIP 配置教程](https://community.st.com/t5/stm32-mcus/how-to-create-a-project-for-stm32h7-with-ethernet-and-lwip-stack/ta-p/49308) 第 7.1 节明确要求将 `DATA_IN_D2_SRAM` 加入工程宏定义；第 4 节给出 H74x/H75x 的 D2 DMA/lwIP 内存布局并说明可采用其他布局。该宏属于 STM32H7 CMSIS 启动和内存时钟配置，不是 lwIP 通用开关，也不能替代 MPU、DMA 可达性、缓存一致性或链接布局检查。教程使用 CubeIDE，Keil 的 Define 是同一预处理宏机制；没有据此引入 RTOS或改动当前固件。原始断言文本未捕获的验证边界仍保留。
- RX 缓冲位于非缓存 D2 SRAM，因此移除了 RX invalidate；TX 每段数据按 32 字节边界清理 DCache。DMA 发送失败和超时会返回 `ERR_IF` 并递增可观察计数器。
- 启动主栈由 1 KiB 增至 4 KiB：Keil 静态调用图估算最深链约 1440 字节，且含未估算函数；map 确认 4 KiB 栈位于 AXI SRAM 且未与 ETH 区域重叠。
- `ethernetif_last_init_error`、HAL/PHY/TX 错误计数、`g_board_phy_status` 和 `g_ch9121_diagnostics` 可供调试器观察。后者新增 `init_stage`、`init_result`、`dhcp_start_result`、`local_port`，分别记录阶段、真实错误和实际监听端口；正常值为 READY(5)、0、0、50000。
- `LWIP_PLATFORM_ASSERT` 已改为记录 `g_lwip_assert_diagnostics.message/file/line` 后进入 `Error_Handler()`，不再通过默认 ARMCC `printf` 进入 semihosting。原调试会话 PC=`0x08000A6C`，与旧 AXF 的 `_sys_open` 中 `BKPT 0xAB` 对应；未捕获触发该路径的原始断言文本。

## 6. 参考例程能提供什么

- `USER/main.c` 提供板级初始化顺序，包括缓存、MPU、PCF8574、网络和周期处理。
- `HARDWARE/LAN8720/lan8720.c/.h` 提供 PHY 地址 0、复位极性和时序、寄存器与链路状态判断；描述符放在 0x30040000，单独配置了 MPU。
- `HARDWARE/PCF8574` 和 `HARDWARE/IIC` 提供复位控制所需的 IO 扩展及 PH4/PH5 操作参考。
- `LWIP/lwip_app/lwip_comm` 提供设备唯一 MAC 生成、默认静态 IP 192.168.1.30，以及 DHCP 失败回退思路。
- 参考 `ethernetif.c` 包含缓存清理/失效处理；该旧实现的 HAL 和收包调用方式与当前版本不同。
- `LWIP/lwip_app/udp_demo` 是普通 **8089 端口、指定远端 IP 的 RAW UDP 文本通信演示**。它调用 `udp_connect()`，不是 CH9121 广播发现或配置协议。
- TCP client/server 和 HTTP 演示属于其他业务参考，当前目标不需要默认接入。

应提取板级事实和实现思路，按当前 lwIP 2.2.1 与 ETH HAL 接口重新适配，不能将旧版 lwIP/HAL 文件直接覆盖进当前工程。

## 7. CH9121 协议适配现状

原工程和正点原子参考例程未提供 CH9121 配置协议；协议实现依据新增的 [`CH9121网络配置协议说明.md`](01_Reference/CH9121网络配置协议说明.md)。固件新增 `LWIP/App/ch9121_service.c/.h`，使用 RAW UDP 50000 接收并由指定 `netif` 广播应答到 `255.255.255.255:60000`，通过 `pbuf_copy_partial()` 支持链式 pbuf。

2026-09-28 调试时纠正了首轮实现的端口字节序错误：`udp_bind()` 和 `udp_sendto_if()` 的端口参数采用主机字节序，现直接传入 50000/60000。旧代码重复 `PP_HTONS()` 导致实际监听 20675、应答目的端口 24810，确实会导致官方工具搜索失败。`LWIP_IP_ACCEPT_UDP_PORT` 接收的是报文中的网络字节序端口，该 hook 仍保留相应转换；两种接口不能混用。

协议固定为 285 字节，标识 `CH9121_CFG_FLAG\0`；搜索 `04→84`、GET `02→82`、SET `01→81`，搜索版本 `0x47`，完整配置版本 `04/07`，无长度外校验尾部。GET 长度 `0xCC`；SET ACK 长度 0 并回显完整数据。配置区按 `74 + 65×2 + 51` 字节组织，只有设备名、DHCP、IP、掩码、网关可修改。名字仅接受 1～20 个可打印 ASCII 字符；静态地址校验连续掩码、设备主机地址和同网段网关。范围外字段变化或参数无效整体返回 `C1`。配置仅存 RAM；DHCP 查询返回实际租约或零地址；SET ACK 发送成功后由主循环切换网络参数。

已新增 `02_Software/tools/ch9121_udp_probe.py`，含扫描、GET、单次 SET 后重新搜索/读回、畸形和不支持 SET 验证，以及写入 1/20 字符名称后恢复原名的边界验证命令。该脚本尚未连板运行。

## 8. 已完成与验证边界

首轮完成：接入 PCF8574/PHY 复位；补齐裸机处理循环、DHCP/静态地址切换、UID 派生 MAC、ETH 错误传播；配置 D2 SRAM MPU/scatter；新增 CH9121 配置服务和 PC UDP 验证脚本；根据 Keil 调用图将启动主栈增至 4 KiB。2026-09-28 后续为排查搜索问题，将启动默认模式改为 DHCP，静态备份地址仍保留。

静态检查：Keil 工程 XML 可解析，工程登记的 112 个文件路径均存在；Python 探测脚本语法、命令帮助、1/20 字符名称边界、DHCP 动态地址比较规则及 285 字节组帧辅助检查通过。Map 显示描述符、RX 池和 heap 区间不重叠。

历史构建：首轮实现与默认 DHCP 版本均通过 Keil UV4 + ARMCC 5.06 update 6 构建。静态调用图最大链估算 1440 字节，未知函数另计；启动栈实际配置 4096 字节。map：RX desc `0x30000000`、TX desc `0x30000080`、RX pool `0x30000100`（`0x3120` 字节）；lwIP heap 固定 `0x30004000`，`MEM_SIZE=16384`；scatter 其余 D2 区从 `0x30009000` 开始。

本次构建：初始化诊断、D2 时钟、端口和断言修复后的最新构建 **0 Error(s), 0 Warning(s)**；Code=52472、RO=1412、RW=13120、ZI=19216。日志为 `02_Software/Demo_Lwip/MDK-ARM/build_init_fix.log`，AXF/HEX 为该工程输出目录中的 `Demo_Lwip.axf/.hex`。

本次直接实机观察：通过 Keil/J-Link 下载修正版，输出 Erase Done / Programming Done / Verify OK；新调试会话运行后暂停位置在 `HAL_ETH_ReadData()`，已进入主循环。读取 `init_stage=5`、`init_result=0`、`dhcp_start_result=0`、`local_port=50000`、`g_lwip_assert_diagnostics.message=0`，`gnetif.flags=0x0F`（接口和链路均 up）。DHCP 实际地址为 **192.168.31.104**，PC Realtek 网卡为 `192.168.31.123/24`、网关 `192.168.31.1`；连续三次 ping 均回复，0% 丢包，0～1 ms。初始化返回成功也表明 PCF8574 ACK 与地址 0 的 PHY ID 校验路径已通过，但未测量复位电气波形或独立记录 PHY ID 数值。

实机协议边界：官方工具窗口操作授权等待超时；工具仍占用 UDP 60000，脚本尝试绑定返回 WinError 10048，因此本次尚未得到脚本搜索/GET/SET 结果。不能把 DHCP/ping 或下载 Verify OK 写成 CH9121 协议验收通过。复位波形、线缆拔插、无 DHCP 时配置、静态切换、官方工具搜索/GET/SET 和边界测试仍待验证。当前板子已恢复连续运行。

## 9. 恢复工作入口与待决事项

1. 当前修正版已下载并获 DHCP 地址 `192.168.31.104`，先在开发板连续运行（Keil F5）时用 `NetModuleConfig_V2.04` 搜索；地址可能随新租约变化。若再停顿，看 `g_ch9121_diagnostics` 和 `g_lwip_assert_diagnostics`；不要仅凭源代码行判断停在 `if`。旧 Keil 会话部分缓存断点出现 illegal qualifier / undefined line number，后续按最新源代码位置重新设置所需断点。
2. 用 `NetModuleConfig_V2.04` 单独完成搜索、GET、设备名/DHCP/IP/掩码/网关修改、重新搜索及读回；官方工具与 Python 脚本不要同时绑定 UDP 60000。
3. Python 脚本命令：`python 02_Software/tools/ch9121_udp_probe.py scan --interface-ip <电脑IPv4> --pc-mac <电脑MAC>`；`get`、`set`、`negative`、`boundary` 子命令使用 `--help` 查看参数。`boundary` 会暂时写入 1/20 字符设备名并恢复原名；`negative` 会发送无副作用的无效 SET 和重复 GET。
4. 实机验证 DHCP 获租约、无 DHCP 服务器仍可广播配置、切回静态、复位恢复 RAM 默认值；保留串口透传和 Flash 保存为后续阶段。

## 10. 操作系统选择讨论（2026-09-28）

用户提出：验证阶段是否需要操作系统，以及 FreeRTOS 是否适合工业环境。

已确认本阶段保持裸机（`NO_SYS=1`），不引入 RTOS：

- 先保持现有 `NO_SYS=1`，使用 lwIP RAW UDP 验证硬件网络、CH9121 查询和修改。当前目标本身不要求 RTOS；先解决 PHY 复位、DMA/缓存和主循环处理等已有问题。
- 裸机需要有界、持续的主循环处理；协议回调不得进行长时间等待。参数保存等耗时操作应转交后台状态机，并测量执行时间和对收包的影响。尤其要核实当前阻塞式发送最坏耗时，而不是宣称使用裸机就自动满足实时性。
- 代码设计时将协议编解码、参数模型、网络收发与参数存储职责分离，方便以后迁移调度方式。若后续明确需要多个独立实时业务、串口透传、采集、日志等并行处理，再评估引入 FreeRTOS；迁移后应重新进行时序和互操作验证。
- 普通工业通信设备可以考虑 FreeRTOS；选型取决于任务复杂度、实时期限和维护要求。RTOS 本身不能解决硬件抗干扰、DMA 缓存一致性、协议边界检查、看门狗策略或断电保存问题，裸机也不自动保证可靠性。
- 若引入 RTOS，可优先静态创建任务/队列、控制内存上限、检查栈余量并配置合理中断优先级。FreeRTOS 对象静态分配不等于 lwIP 内部不再分配 pbuf/heap。
- 当前 RAW UDP 接口不是可随意由多个任务同时调用的接口。后续如引入 lwIP OS 模式，需遵守其核心线程/核心锁规则，并明确 `sys_arch`、收包、定时器和 API 调用上下文的适配方案。
- 工业环境与功能安全认证要求应分别讨论。若涉及安全联锁、急停或其他安全功能，要核实具体内核版本、CPU 移植、工具链和认证材料及系统级安全设计；不能把普通 FreeRTOS 配置等同于认证方案，也不能笼统认定 FreeRTOS 永远没有认证相关路径。

本轮核对的官方资料：

- [lwIP 主循环模式](https://www.nongnu.org/lwip/2_0_x/group__lwip__nosys.html)：裸机模式、主循环递交数据及周期处理定时器。
- [lwIP 多线程规则](https://www.nongnu.org/lwip/2_1_x/multithreading.html)：RAW API 的调用上下文与线程安全限制。
- [FreeRTOS 静态/动态分配](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/09-Memory-management/03-Static-vs-Dynamic-memory-allocation)：任务、队列等对象可静态创建。
- [FreeRTOS 官方功能安全讨论](https://forums.freertos.org/t/freertos-mpu-and-functional-safety/19274/2)：功能安全需结合系统架构与验证。
- [FreeRTOS 安全认证工作跟踪](https://github.com/FreeRTOS/FreeRTOS-Kernel/issues/906)：具体认证支持应按采用版本和移植核实，不按笼统产品名称判断。

本阶段未引入 RTOS；固件已修改并通过 Keil 编译和 map 检查。后续实机节点已确认 DHCP/ping；PHY 复位波形、线缆恢复和 CH9121 官方工具互操作仍未验收。

## 11. 节点记录

| 日期 | 节点 | 结果 |
| --- | --- | --- |
| 2026-09-28 | 初始阅读与交接建立 | 已梳理当前 CubeMX 裸机工程、正点原子参考例程和原理图；确认无 CH9121 协议实现，并记录网络基础缺口；待确认协议角色后共同制定计划 |
| 2026-09-28 | 操作系统选择讨论 | 建议先裸机验证 CH9121 协议，根据后续并行业务与实时期限再评估 FreeRTOS；工业可靠性与功能安全要求分开判断；正式选型待用户确认，固件未改 |
| 2026-09-28 | CH9121 裸机实现计划确认 | 确认设备端、官方 NetModuleConfig V2.04、五个可修改字段、版本 0x47、RAM-only、ACK 后应用；拒绝范围外 SET；新增本地协议说明和上位机；开始固件实施 |
| 2026-09-28 | 裸机网络与 CH9121 首轮实现 | 接入 PCF8574 PHY 复位、UID MAC、有界 lwIP 主循环、D2 DMA 内存布局与错误计数；加入 RAW UDP 搜索/GET/SET、五项校验、DHCP 应用状态和 PC 探测脚本；主栈扩至 4 KiB。Keil ARMCC 完整构建 0 错误/0 警告，map 地址与区域无重叠；实机及上位机验收待进行 |
| 2026-09-28 | NetModuleCfgTool 搜索为空初次诊断 | 用户截图显示搜索结束但列表为空，当时仅确认生成 HEX，无下载/调试器记录。此节点原先认定代码实际绑定 50000、应答 60000 的结论有误，后续检查发现 RAW UDP 参数重复字节序转换，已在初始化排查节点纠正；搜索 04→84 编解码结论保留 |
| 2026-09-28 | 为搜索排查改为默认 DHCP | 用户要求上电默认 DHCP 以观察搜索效果。服务启动时先保持 IPv4 为 0 并调用 `dhcp_start()`；保留静态备份 `192.168.1.30/24`、网关 `192.168.1.1`，有效 SET 可切换回静态。Keil UV4/ARMCC 5.06 update 6 本次完整构建 0 错误/0 警告，并重新生成 HEX；尚未下载或实机验证。若无 DHCP 租约，搜索应仍走 UDP 50000 广播接收路径，但应实机确认上位机能否接收零地址应答 |
| 2026-09-28 | 初始化停顿与搜索端口修复 | 旧调试 PC 停在 `_sys_open` 的 BKPT 0xAB；纠正 lwIP 断言的 printf/semihosting 路径，保存断言信息；提前开启 D2 SRAM1/2/3 时钟；修复 RAW UDP 主机字节序参数；增加初始化阶段与真实返回值。最新构建 0 错误/0 警告；修正版 J-Link 下载 Verify OK；板上 READY=5、初始化/DHCP=0、端口 50000、未触发断言，获得 DHCP 192.168.31.104，ping 3/3 成功。广播协议验收待进行，官方工具占用 60000 阻止脚本绑定 |
