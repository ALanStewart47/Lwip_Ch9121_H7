/*
*********************************************************************************************************
*
*	模块名称 : SPI桥接应用层
*	文件名称 : app_spi_bridge.h
*	版    本 : V1.0
*	说    明 : 处理串口到SPI从机的数据转发业务
*
*	Copyright (C), 2025, CST
*
*********************************************************************************************************
*/

#ifndef __APP_SPI_BRIDGE_H
#define __APP_SPI_BRIDGE_H

#include "main.h"
#include "bsp_spi_bus_dma.h"

// SPI桥接状态
typedef enum {
    SPI_BRIDGE_IDLE = 0,     // 空闲
    SPI_BRIDGE_BUSY,         // 忙碌（正在处理SPI通讯）
    SPI_BRIDGE_ERROR         // 错误状态
} spi_bridge_status_t;



// SPI桥接上下文
typedef struct {
    spi_bridge_status_t status;     // 当前状态
    uint8_t slave_id;               // 目标从机ID
    uint32_t start_tick;            // 开始时间
    uint8_t retry_count;            // 重试次数
    uint8_t *send_data;          // 请求数据指针
    uint16_t send_len;           // 请求数据长度
    uint16_t receive_len;          // 期望的应答长度
    uint32_t delay_ms;              // 请求后等待时间(毫秒)
    uint32_t timeout_ms;            // 超时时间(毫秒)
} spi_master_config_t;



// 配置参数
#define SPI_BRIDGE_MAX_RETRY        3           // 最大重试次数
#define SPI_BRIDGE_DEFAULT_DELAY    2           // 默认延时2ms
#define SPI_BRIDGE_DEFAULT_TIMEOUT  100         // 默认超时100ms
#define SPI_BRIDGE_DEFAULT_RESPONSE_LEN 10      // 默认应答长度

// 最大重试次数
#define SPI_BRIDGE_MAX_RETRY_COUNT      3
// 默认超时时间（毫秒）
#define SPI_MASTER_DEFAULT_TIMEOUT_MS   50
// 默认延时时间（毫秒）
#define SPI_MASTER_DEFAULT_DELAY_MS     2



// 默认应答长度（字节）
#define SPI_MASTER_DEFAULT_RESP_LEN     23
// 应用层额外超时缓冲（毫秒）
#define SPI_MASTER_APP_TIMEOUT_BUFFER   1000

// ================= 核心接口 =================

/**
 * @brief 初始化SPI桥接模块
 * @param void
 * @retval void
 */
void app_spi_bridge_init(void);

/**
 * @brief 设置SPI桥接配置
 * @param uart_number: 串口号
 * @param slave_id: 从机ID
 * @param response_len: 期望应答长度
 * @param delay_ms: 延时时间(毫秒)
 * @param timeout_ms: 超时时间(毫秒)
 * @retval spi_status_t
 */
spi_status_t app_spi_receive_config(
                                    uint8_t slave_id,
                                    uint16_t receive_len,
                                    uint32_t delay_ms,
                                    uint32_t timeout_ms);

/**
 * @brief 发送数据到SPI从机
 * @param uart_number: 串口号
 * @param data: 要发送的数据
 * @param len: 数据长度
 * @retval spi_status_t
 */
spi_status_t app_spi_send(uint8_t slave_id, uint8_t *data, uint16_t len);

/**
 * @brief 获取SPI桥接状态
 * @param uart_number: 串口号
 * @retval spi_bridge_status_t
 */
spi_bridge_status_t app_spi_slave_get_status(uint8_t slave_id);

/**
 * @brief SPI桥接处理函数（在主循环中调用）
 * @param void
 * @retval void
 */
void app_spi_master_process(void);

/**
 * @brief 中止指定串口的SPI通讯
 * @param uart_number: 串口号
 * @retval spi_status_t
 */
spi_status_t app_spi_master_abort(uint8_t slave_id);

// ================= 回调函数 =================

/**
 * @brief SPI通讯完成回调函数
 * @param slave_id: 从机ID
 * @param status: 通讯状态
 * @param response_data: 应答数据
 * @param response_len: 应答数据长度
 * @retval void
 */
void app_spi_master_comm_callback(
                                    uint8_t slave_id, 
                                    spi_status_t status, 
                                    uint8_t *response_data, 
                                    uint16_t receive_len);

#endif

/***************************** CST (END OF FILE) *********************************/
