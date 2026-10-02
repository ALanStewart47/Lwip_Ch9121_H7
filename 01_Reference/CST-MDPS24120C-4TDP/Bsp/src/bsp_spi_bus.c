/*
*********************************************************************************************************
*
*	模块名称 : SPI总线驱动
*	文件名称 : bsp_spi_bus.c
*	版    本 : V2.0
*	说    明 : 精简同步接口，支持多机通讯
*
*	Copyright (C), 2020-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp_spi_bus.h"
#include <string.h>

/*
*********************************************************************************************************
*	                             选择DMA，中断或者查询方式
*********************************************************************************************************
*/
#define USE_SPI_DMA    /* DMA方式  */
//#define USE_SPI_INT    /* 中断方式 */
//#define USE_SPI_POLL   /* 查询方式 */

enum {
	TRANSFER_WAIT,
	TRANSFER_COMPLETE,
	TRANSFER_ERROR
};

/*
*********************************************************************************************************
*	                                           变量
*********************************************************************************************************
*/

static const uint16_t user_spi_nss_gpio_pin[SLAVE_SPI_MAX_NUM] = {
    SLAVE_ENABLE_1_Pin, SLAVE_ENABLE_2_Pin,
    SLAVE_ENABLE_3_Pin, SLAVE_ENABLE_4_Pin,
    SLAVE_ENABLE_5_Pin, SLAVE_ENABLE_6_Pin,
    SLAVE_ENABLE_7_Pin, SLAVE_ENABLE_8_Pin
};

static GPIO_TypeDef * const user_spi_nss_gpio_port[SLAVE_SPI_MAX_NUM] = {
    SLAVE_ENABLE_1_GPIO_Port, SLAVE_ENABLE_2_GPIO_Port,
    SLAVE_ENABLE_3_GPIO_Port, SLAVE_ENABLE_4_GPIO_Port,
    SLAVE_ENABLE_5_GPIO_Port, SLAVE_ENABLE_6_GPIO_Port,
    SLAVE_ENABLE_7_GPIO_Port, SLAVE_ENABLE_8_GPIO_Port
};

static uint8_t g_spi_busy = 0;
__IO uint32_t wTransferState = TRANSFER_WAIT;

extern SPI_HandleTypeDef hspi3;

// DMA缓冲区
uint8_t g_spiTxBuf[SLAVE_SPI_MAX_NUM][SPI_BUFFER_SIZE];  
uint8_t g_spiRxBuf[SLAVE_SPI_MAX_NUM][SPI_BUFFER_SIZE];

/*
*********************************************************************************************************
*	函 数 名: bsp_InitSPIBus
*	功能说明: 配置SPI总线。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitSPIBus(void)
{	
	g_spi_busy = 0;
}

/*
*********************************************************************************************************
*	函 数 名: HAL_SPI_TxRxCpltCallback，HAL_SPI_ErrorCallback
*	功能说明: SPI数据传输完成回调和传输错误回调
*	形    参: SPI_HandleTypeDef 类型指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &hspi3)
    {
		wTransferState = TRANSFER_COMPLETE;
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &hspi3)
		wTransferState = TRANSFER_COMPLETE;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &hspi3){
		wTransferState = TRANSFER_ERROR;
    }
}

/*
*********************************************************************************************************
*	                                 内部辅助函数
*********************************************************************************************************
*/

// 内部：归一化超时（0->默认）
static inline uint32_t spi_norm_timeout(uint32_t timeout_ms) {
    return (timeout_ms == 0) ? SPI_DEFAULT_TIMEOUT_MS : timeout_ms;
}

// 内部：总线占用/释放
static void spi_bus_enter(void) {
    g_spi_busy = 1;
}

static void spi_bus_exit(void) {
    g_spi_busy = 0;
}

static uint8_t spi_bus_busy(void) {
    return g_spi_busy;
}

// 内部：等待DMA完成（带超时），转换为 spi_status_t
static spi_status_t spi_wait_done(uint32_t timeout_ms) 
{
    uint32_t start = HAL_GetTick();
    while (wTransferState == TRANSFER_WAIT) 
    {
        if ((HAL_GetTick() - start) >= timeout_ms) {
            return SPI_STATUS_TIMEOUT;
        }
    }
    return (wTransferState == TRANSFER_COMPLETE) ? SPI_STATUS_OK : SPI_STATUS_ERROR;
}

/*
*********************************************************************************************************
*	                                 核心接口实现
*********************************************************************************************************
*/

// 工具：给定起始tick与超时阈值，返回是否超时（1=超时/0=未超时）
uint8_t bsp_spi_timed_out(uint32_t start_tick, uint32_t timeout_ms) {
    if (timeout_ms == 0) timeout_ms = SPI_DEFAULT_TIMEOUT_MS;
    return (HAL_GetTick() - start_tick) >= timeout_ms;
}

// 交换（TX/RX 同步进行）：常用于发送后读取回应
spi_status_t bsp_spi_transfer(uint8_t slave_id, const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_ms)
{
    if (slave_id >= SLAVE_SPI_MAX_NUM) return SPI_STATUS_INVALID_PARAM;
    if (len == 0 || len > SPI_BUFFER_SIZE) return SPI_STATUS_INVALID_PARAM;

    timeout_ms = spi_norm_timeout(timeout_ms);

    // 进入总线
    if (spi_bus_busy()) return SPI_STATUS_BUSY;
    spi_bus_enter();

    // 填充发送缓冲
    if (tx) {
        memcpy(g_spiTxBuf[slave_id], tx, len);
    } else {
        memset(g_spiTxBuf[slave_id], 0x00, len);
    }

    // 片选拉低
    HAL_GPIO_WritePin(user_spi_nss_gpio_port[slave_id], user_spi_nss_gpio_pin[slave_id], GPIO_PIN_RESET);

#ifdef USE_SPI_DMA
    wTransferState = TRANSFER_WAIT;
    if (HAL_SPI_TransmitReceive_DMA(&hspi3, g_spiTxBuf[slave_id], g_spiRxBuf[slave_id], len) != HAL_OK) 
    {
        HAL_GPIO_WritePin(user_spi_nss_gpio_port[slave_id], user_spi_nss_gpio_pin[slave_id], GPIO_PIN_SET);
        spi_bus_exit();
        return SPI_STATUS_ERROR;
    }
    spi_status_t st = spi_wait_done(timeout_ms);
#elif defined(USE_SPI_INT)
    wTransferState = TRANSFER_WAIT;
    if (HAL_SPI_TransmitReceive_IT(&hspi3, g_spiTxBuf[slave_id], g_spiRxBuf[slave_id], len) != HAL_OK) {
        HAL_GPIO_WritePin(user_spi_nss_gpio_port[slave_id], user_spi_nss_gpio_pin[slave_id], GPIO_PIN_SET);
        spi_bus_exit();
        return SPI_STATUS_ERROR;
    }
    spi_status_t st = spi_wait_done(timeout_ms);
#else
    HAL_StatusTypeDef hs = HAL_SPI_TransmitReceive(&hspi3, g_spiTxBuf[slave_id], g_spiRxBuf[slave_id], len, timeout_ms);
    spi_status_t st = (hs == HAL_OK) ? SPI_STATUS_OK : SPI_STATUS_ERROR;
#endif

    // 片选拉高
    HAL_GPIO_WritePin(user_spi_nss_gpio_port[slave_id], user_spi_nss_gpio_pin[slave_id], GPIO_PIN_SET);

    // 输出数据
    if (st == SPI_STATUS_OK && rx) {
        memcpy(rx, g_spiRxBuf[slave_id], len);
    }

    spi_bus_exit();
    return st;
}

// 仅发送（忽略接收）
spi_status_t bsp_spi_send(uint8_t slave_id, const uint8_t *tx, uint16_t len, uint32_t timeout_ms)
{
    return bsp_spi_transfer(slave_id, tx, NULL, len, timeout_ms);
}

// 仅接收（主机发0x00产生时钟）
spi_status_t bsp_spi_recv(uint8_t slave_id, uint8_t *rx, uint16_t len, uint32_t timeout_ms)
{
    return bsp_spi_transfer(slave_id, NULL, rx, len, timeout_ms);
}
	
/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
