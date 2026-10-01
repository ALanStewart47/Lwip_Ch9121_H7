/*
*********************************************************************************************************
*
*	模块名称 : SPI总线驱动
*	文件名称 : bsp_spi_bus.h
*	版    本 : V2.0
*	说    明 : 精简同步接口，支持多机通讯
*
*	Copyright (C), 2020-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "main.h"

#ifndef __BSP_SPI_BUS_H
#define __BSP_SPI_BUS_H

// SPI 状态枚举
typedef enum {
    SPI_STATUS_OK = 0,
    SPI_STATUS_ERROR,
    SPI_STATUS_TIMEOUT,
    SPI_STATUS_BUSY,
    SPI_STATUS_INVALID_PARAM
} spi_status_t;

#define SPI_BUFFER_SIZE         300
#define SPI_DEFAULT_TIMEOUT_MS  100
#define SLAVE_SPI_MAX_NUM       8

// ================= 核心同步接口 =================
// 发送：向从机写入数据（不关心RX），timeout_ms==0 时使用默认超时
spi_status_t bsp_spi_send(uint8_t slave_id, const uint8_t *tx, uint16_t len, uint32_t timeout_ms);

// 接收：从从机读取数据（主机发送0x00产生时钟），timeout_ms==0 时使用默认超时
spi_status_t bsp_spi_recv(uint8_t slave_id, uint8_t *rx, uint16_t len, uint32_t timeout_ms);

// 交换：TX/RX 同步进行，常用于发送后读取回应，timeout_ms==0 时使用默认超时
spi_status_t bsp_spi_transfer(uint8_t slave_id, const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_ms);

// 超时判断工具：给定起始tick与超时阈值，返回是否超时（1=超时/0=未超时）
uint8_t bsp_spi_timed_out(uint32_t start_tick, uint32_t timeout_ms);

// 初始化
void bsp_InitSPIBus(void);

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
