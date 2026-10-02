
#include "app_spi_bridge.h"
#include "app_uart.h"
#include "bsp_spi_bus_dma.h"
#include <string.h>

/*
*********************************************************************************************************
*	                                           变量
*********************************************************************************************************
*/

// 支持多个串口的SPI桥接上下文
spi_master_config_t g_spi_master_ctx[SLAVE_SPI_MAX_NUM];


/*
*********************************************************************************************************
*	                                 核心接口实现
*********************************************************************************************************
*/

/**
 * @brief 初始化SPI桥接模块
 */
void app_spi_bridge_init(void)
{
    // 清零所有桥接上下文
    memset(g_spi_master_ctx, 0, sizeof(g_spi_master_ctx));
    
    // 初始化默认配置
    for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++) 
    {
        g_spi_master_ctx[i].status       = SPI_BRIDGE_IDLE;
        g_spi_master_ctx[i].slave_id     = i;  
        g_spi_master_ctx[i].receive_len = SPI_MASTER_DEFAULT_RESP_LEN;
        g_spi_master_ctx[i].delay_ms     = SPI_MASTER_DEFAULT_DELAY_MS;
        g_spi_master_ctx[i].timeout_ms   = SPI_MASTER_DEFAULT_TIMEOUT_MS;
        g_spi_master_ctx[i].retry_count  = SPI_BRIDGE_MAX_RETRY_COUNT;
    }
    bsp_InitSPIBus();
}

/**
 * @brief 设置SPI桥接配置
 */
spi_status_t app_spi_receive_config(uint8_t slave_id,
                                   uint16_t receive_len,
                                   uint32_t delay_ms,
                                   uint32_t timeout_ms)
{
    spi_master_config_t *ctx = &g_spi_master_ctx[slave_id];
    if (ctx == NULL) {
        return SPI_STATUS_INVALID_PARAM;
    }
    
    // 检查从机ID是否有效
    if (slave_id >= SLAVE_SPI_MAX_NUM) {
        return SPI_STATUS_INVALID_PARAM;
    }
    // 更新配置
    ctx->slave_id       = slave_id;
    ctx->receive_len   = (receive_len > 0) ? receive_len : SPI_MASTER_DEFAULT_RESP_LEN;
    ctx->delay_ms       = delay_ms;
    ctx->timeout_ms     = (timeout_ms > 0) ? timeout_ms : SPI_MASTER_DEFAULT_TIMEOUT_MS;
    return SPI_STATUS_OK;
}

/**
 * @brief 发送数据到SPI从机
 */
spi_status_t app_spi_send(uint8_t slave_id, uint8_t *data, uint16_t len)
{
    spi_master_config_t *ctx =  &g_spi_master_ctx[slave_id];
    if (ctx == NULL || data == NULL || len == 0) {
        return SPI_STATUS_INVALID_PARAM;
    }
    
    // 检查当前状态
    if (ctx->status == SPI_BRIDGE_BUSY) {
        return SPI_STATUS_BUSY;
    }
    
    // 更新上下文
    ctx->status         = SPI_BRIDGE_BUSY;
    ctx->start_tick     = HAL_GetTick();
    ctx->retry_count    = 0;
    ctx->send_data   = data;
    ctx->send_len    = len;
    
    // 发起SPI通讯(将毫秒转换为tick，假设TIM7为1ms周期)
    spi_status_t result = bsp_spi_comm(
        slave_id,
        data,
        len,
        ctx->receive_len,
        ctx->delay_ms,        // 直接传入，底层会转换为tick
        ctx->timeout_ms,      // 直接传入，底层会转换为tick
        app_spi_master_comm_callback
    );
    
    // 如果发起失败，重置状态
    if (result != SPI_STATUS_OK) {
        ctx->status = SPI_BRIDGE_IDLE;
    }
    return result;
}

/**
 * @brief 获取SPI桥接状态
 */
spi_bridge_status_t app_spi_slave_get_status(uint8_t slave_id)
{
    spi_master_config_t *ctx =  &g_spi_master_ctx[slave_id];
    if (ctx == NULL) 
    {
        return SPI_BRIDGE_ERROR;
    }
    
    return ctx->status;
}

/**
 * @brief SPI桥接处理函数（在主循环中调用）
 */
void app_spi_master_process(void)
{
    bsp_spi_process();
    
    // 检查超时和重试逻辑
    for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++) 
    {
        spi_master_config_t *ctx = &g_spi_master_ctx[i];
        
        if (ctx->status == SPI_BRIDGE_BUSY) 
        {
            // 检查是否超时（这里是额外的应用层超时检查）
            uint32_t elapsed = HAL_GetTick() - ctx->start_tick;
            if (elapsed > (ctx->timeout_ms + SPI_MASTER_APP_TIMEOUT_BUFFER)) 
            { // 额外缓冲
                // 应用层超时，强制重置
                ctx->status = SPI_BRIDGE_ERROR;
                printf("slave %d send timeout", i);
                // 可以在这里添加错误处理逻辑
            }
        }
    }
}

/**
 * @brief 中止指定串口的SPI通讯
 */
spi_status_t app_spi_master_abort(uint8_t slave_id)
{
    spi_master_config_t *ctx = &g_spi_master_ctx[slave_id];

    if (ctx == NULL) {
        return SPI_STATUS_INVALID_PARAM;
    }
    
    // 中止底层SPI通讯
    spi_status_t result = bsp_spi_abort(ctx->slave_id);
    
    // 重置上下文状态
    ctx->status = SPI_BRIDGE_IDLE;
    ctx->retry_count = 0;
    
    return result;
}

/*
*********************************************************************************************************
*	                                 回调函数实现
*********************************************************************************************************
*/

/**
 * @brief SPI通讯完成回调函数
 */

void app_spi_master_comm_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len)
{
    // 根据从机ID查找对应的串口
    
    spi_master_config_t *ctx = &g_spi_master_ctx[slave_id];
    if (ctx == NULL) {
        return;
    }
    
    switch (status) 
    {
        case SPI_STATUS_OK:
            // 通讯成功，发送应答数据给上位机
            if (response_data != NULL && receive_len > 0) 
            {
				//memcpy(tcp_send,response_data,receive_len);
                //send(1,tcp_send,receive_len);

                
                DMA_Usart_Send(0, response_data, receive_len);
            }
            ctx->status = SPI_BRIDGE_IDLE;
            ctx->retry_count = 0;
            break;
            
        case SPI_STATUS_TIMEOUT:
        case SPI_STATUS_ERROR:
            // 通讯失败，检查是否需要重试
            #if SPI_BRIDGE_ENABLE_RETRY
            if (ctx->retry_count < SPI_BRIDGE_MAX_RETRY_COUNT) {
                ctx->retry_count++;
               // SPI_BRIDGE_DEBUG("UART%d retry %d/%d", uart_number, ctx->retry_count, SPI_BRIDGE_MAX_RETRY_COUNT);
                // 重试发送
                spi_status_t retry_result = bsp_spi_comm(
                    slave_id,
                    ctx->send_data,
                    ctx->send_len,
                    ctx->config.receive_len,
                    ctx->config.delay_ms,
                    ctx->config.timeout_ms,
                    app_spi_bridge_comm_callback
                );
                
                if (retry_result != SPI_STATUS_OK) {
                    // 重试失败，标记为错误
                    ctx->status = SPI_BRIDGE_ERROR;
                    #if SPI_BRIDGE_ENABLE_ERROR_RESP
                    SPI_BRIDGE_SEND_ERROR(uart_number, SPI_BRIDGE_ERROR_CODE_RETRY);
                    #endif
                }
            } else 
            #endif
            {
                // 重试次数用完或未启用重试，标记为错误
                ctx->status = SPI_BRIDGE_ERROR;
               // SPI_BRIDGE_DEBUG("UART%d communication failed", uart_number);
                #if SPI_BRIDGE_ENABLE_ERROR_RESP
                uint8_t error_code = (status == SPI_STATUS_TIMEOUT) ? 
                                   SPI_BRIDGE_ERROR_CODE_TIMEOUT : SPI_BRIDGE_ERROR_CODE_RETRY;
                SPI_BRIDGE_SEND_ERROR(uart_number, error_code);
                #endif
            }
            break;
            
        case SPI_STATUS_ABORTED:
            // 通讯被中止
            ctx->status = SPI_BRIDGE_IDLE;
            ctx->retry_count = 0;
            break;
            
        default:
            ctx->status = SPI_BRIDGE_ERROR;
            break;
    }
}

/***************************** CST (END OF FILE) *********************************/
