/*
*********************************************************************************************************
*
*	文件名称 : app_spi_bridge_usage.c
*	版    本 : V1.0
*	说    明 : SPI桥接模块使用示例
*
*	Copyright (C), 2025, CST
*
*********************************************************************************************************
*/

#include "app_spi_bridge.h"
#include "app_uart.h"

/**
 * @brief SPI桥接使用示例1：基本的串口到SPI转发
 * @note 这个示例展示了如何设置基本的串口到SPI从机的数据转发
 */
void example_basic_bridge(void)
{
    // 1. 初始化SPI桥接模块（通常在系统初始化时调用一次）
    app_spi_bridge_init();
    
    // 2. 配置串口0到从机0的桥接
    // 参数：串口号0，从机ID0，期望应答长度10字节，延时2ms，超时100ms
    app_spi_bridge_config(0, 0, 10, 2, 100);
    
    // 3. 在主循环中处理桥接任务
    while(1) {
        // 处理SPI桥接业务（这会自动处理串口数据的转发）
        app_spi_bridge_process();
        
        // 其他业务逻辑...
        
        // 延时
        HAL_Delay(1);
    }
}

/**
 * @brief SPI桥接使用示例2：多串口多从机配置
 */
void example_multi_bridge(void)
{
    // 初始化
    app_spi_bridge_init();
    
    // 配置多个串口到不同从机的桥接
    app_spi_bridge_config(0, 0, 10, 2, 100);  // 串口0 -> 从机0
    app_spi_bridge_config(1, 1, 20, 5, 200);  // 串口1 -> 从机1
    
    // 主循环
    while(1) {
        app_spi_bridge_process();
        HAL_Delay(1);
    }
}

/**
 * @brief SPI桥接使用示例3：手动发送数据到指定从机
 */
void example_manual_send(void)
{
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    
    // 手动发送数据到从机
    spi_status_t result = app_spi_bridge_send(0, test_data, sizeof(test_data));
    
    if (result == SPI_STATUS_OK) {
        // 发送成功，数据已进入发送队列
        // 应答数据会通过回调函数自动发送给串口
    } else if (result == SPI_STATUS_BUSY) {
        // SPI忙碌，可以稍后重试
    } else {
        // 发送失败
    }
}

/**
 * @brief SPI桥接使用示例4：状态监控和错误处理
 */
void example_status_monitoring(void)
{
    while(1) {
        // 检查各个串口的桥接状态
        for (uint8_t i = 0; i < UART_SUM; i++) {
            spi_bridge_status_t status = app_spi_bridge_get_status(i);
            
            switch(status) {
                case SPI_BRIDGE_IDLE:
                    // 空闲状态，可以发送新数据
                    break;
                    
                case SPI_BRIDGE_BUSY:
                    // 忙碌状态，正在处理SPI通讯
                    break;
                    
                case SPI_BRIDGE_ERROR:
                    // 错误状态，可能需要重置或处理
                    app_spi_bridge_abort(i);  // 中止当前操作
                    // 重新配置或记录错误日志...
                    break;
            }
        }
        
        app_spi_bridge_process();
        HAL_Delay(10);
    }
}

/**
 * @brief SPI桥接使用示例5：动态配置不同的应答长度
 */
void example_dynamic_config(void)
{
    // 根据不同的命令，配置不同的应答长度
    uint8_t command_type = 0x01; // 假设从串口数据中解析出的命令类型
    
    switch(command_type) {
        case 0x01:
            // 状态查询命令，期望4字节应答
            app_spi_bridge_config(0, 0, 4, 2, 50);
            break;
            
        case 0x02:
            // 数据读取命令，期望64字节应答
            app_spi_bridge_config(0, 0, 64, 5, 200);
            break;
            
        case 0x03:
            // 配置命令，期望2字节确认应答
            app_spi_bridge_config(0, 0, 2, 1, 30);
            break;
            
        default:
            // 默认配置
            app_spi_bridge_config(0, 0, 10, 2, 100);
            break;
    }
}

/**
 * @brief 集成到现有uart_task中的示例
 */
void uart_task_with_bridge(void)
{
    // 这个函数展示了如何将SPI桥接集成到现有的uart_task中
    
    // 处理SPI桥接业务（必须在主循环中调用）
    app_spi_bridge_process();
    
    // 处理串口数据（uart_data_handle中会自动调用SPI桥接功能）
    uart_data_handle(&uart_data[0]);
    
    // 如果有多个串口
    #if GD32
    uart_data_handle(&uart_data[1]);
    #endif
}

/***************************** CST (END OF FILE) *********************************/
