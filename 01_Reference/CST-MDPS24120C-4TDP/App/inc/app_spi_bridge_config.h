/*
*********************************************************************************************************
*
*	模块名称 : SPI桥接配置
*	文件名称 : app_spi_bridge_config.h
*	版    本 : V1.0
*	说    明 : SPI桥接模块的配置参数和预定义设置
*
*	Copyright (C), 2025, CST
*
*********************************************************************************************************
*/

#ifndef __APP_SPI_BRIDGE_CONFIG_H
#define __APP_SPI_BRIDGE_CONFIG_H

// ================= 基本配置参数 =================

// 最大重试次数
#define SPI_BRIDGE_MAX_RETRY_COUNT      3

// 默认超时时间（毫秒）
#define SPI_BRIDGE_DEFAULT_TIMEOUT_MS   20

// 默认延时时间（毫秒）
#define SPI_BRIDGE_DEFAULT_DELAY_MS     10

// 默认应答长度（字节）
#define SPI_BRIDGE_DEFAULT_RESP_LEN     300

// 应用层额外超时缓冲（毫秒）
#define SPI_BRIDGE_APP_TIMEOUT_BUFFER   1000

// ================= 预定义从机配置 =================

// 从机0配置（默认从机）
#define SLAVE_0_RESPONSE_LEN            300
#define SLAVE_0_DELAY_MS                2
#define SLAVE_0_TIMEOUT_MS              100

// 从机1配置（高速从机）
#define SLAVE_1_RESPONSE_LEN            300
#define SLAVE_1_DELAY_MS                1
#define SLAVE_1_TIMEOUT_MS              50

// 从机2配置（慢速从机）
#define SLAVE_2_RESPONSE_LEN            64
#define SLAVE_2_DELAY_MS                10
#define SLAVE_2_TIMEOUT_MS              500

// 从机3配置（传感器从机）
#define SLAVE_3_RESPONSE_LEN            8
#define SLAVE_3_DELAY_MS                5
#define SLAVE_3_TIMEOUT_MS              200

// ================= 错误码定义 =================

// 应用层错误码
#define SPI_BRIDGE_ERROR_CODE_TIMEOUT   0xFE
#define SPI_BRIDGE_ERROR_CODE_RETRY     0xFD
#define SPI_BRIDGE_ERROR_CODE_BUSY      0xFC
#define SPI_BRIDGE_ERROR_CODE_PARAM     0xFB

// ================= 功能开关 =================

// 是否启用重试机制
#define SPI_BRIDGE_ENABLE_RETRY         1

// 是否启用错误响应发送
#define SPI_BRIDGE_ENABLE_ERROR_RESP    1

// 是否启用状态监控
#define SPI_BRIDGE_ENABLE_STATUS_MON    1

// 是否启用调试信息
#define SPI_BRIDGE_ENABLE_DEBUG         0

// ================= 调试宏定义 =================

#if SPI_BRIDGE_ENABLE_DEBUG
    #include <stdio.h>
    #define SPI_BRIDGE_DEBUG(fmt, ...) printf("[SPI_BRIDGE] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define SPI_BRIDGE_DEBUG(fmt, ...)
#endif

// ================= 预定义配置结构 =================

// 快速配置宏
#define SPI_BRIDGE_CONFIG_DEFAULT(uart, slave) \
    (uart, slave, SPI_BRIDGE_DEFAULT_RESP_LEN, \
                         SPI_BRIDGE_DEFAULT_DELAY_MS, SPI_BRIDGE_DEFAULT_TIMEOUT_MS)

#define SPI_BRIDGE_CONFIG_FAST(uart, slave) \
    app_spi_bridge_config(uart, slave, 4, 1, 30)

#define SPI_BRIDGE_CONFIG_SLOW(uart, slave) \
    app_spi_bridge_config(uart, slave, 32, 10, 300)

#define SPI_BRIDGE_CONFIG_SENSOR(uart, slave) \
    app_spi_bridge_config(uart, slave, SLAVE_##slave##_RESPONSE_LEN, \
                         SLAVE_##slave##_DELAY_MS, SLAVE_##slave##_TIMEOUT_MS)

// ================= 错误处理宏 =================

#if SPI_BRIDGE_ENABLE_ERROR_RESP
    #define SPI_BRIDGE_SEND_ERROR(uart, code) do { \
        uint8_t error_resp[] = {0xFF, code}; \
        DMA_Usart_Send(uart, error_resp, sizeof(error_resp)); \
    } while(0)
#else
    #define SPI_BRIDGE_SEND_ERROR(uart, code)
#endif

#endif

/***************************** CST (END OF FILE) *********************************/
