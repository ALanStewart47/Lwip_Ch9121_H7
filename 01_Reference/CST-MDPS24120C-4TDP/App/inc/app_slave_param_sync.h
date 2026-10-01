/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_slave_param_sync.h
 *
 * @par dependencies
 * - stdint.h
 * - stdbool.h
 *
 * @author ALan | R&D Dept. | CST
 *
 * @brief Slave parameter synchronization module
 *
 * Processing flow:
 * 1. Background periodic query (200-500ms interval)
 * 2. Cache slave parameters locally
 * 3. Support display and control 1-8 slaves (32 channels total)
 *
 * @version V1.0 2025-10-30  ALan  Initial version
 * @note 
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __APP_SLAVE_PARAM_SYNC_H__
#define __APP_SLAVE_PARAM_SYNC_H__

//******************************** Includes *********************************//
#include <stdint.h>
#include <stdbool.h>
#include "app_light.h"
#ifdef __cplusplus
extern "C" {
#endif
//******************************** Includes *********************************//

//******************************** Defines **********************************//
// 查询间隔配置
#define SLAVE_QUERY_INTERVAL_MS         50      // 50ms quary one slave 
#define SLAVE_QUERY_TIMEOUT_MS          6       // 单次查询超时6ms
#define SLAVE_QUERY_RETRY_MAX           3       // 查询失败重试3次
#define SLAVE_QUERY_WAITING_TIME        500    //500

#define ROUTER_ACTIVITY_HISTORY_SIZE    5   // 记录最近5次活动

#define SLAVE_PARAM_SYNC_RETRIES_MAX    3   

// Extended function codes - parameter query and set
typedef enum {
    CMD_PARAM_GET_BRIGHTNESS    = 0x10,  // Query brightness
    CMD_PARAM_SET_BRIGHTNESS    = 0x11,  // Set brightness
    CMD_PARAM_GET_STROBE_WIDTH  = 0x12,  // Query strobe width
    CMD_PARAM_SET_STROBE_WIDTH  = 0x13,  // Set strobe width
    CMD_PARAM_GET_WORK_MODE     = 0x14,  // Query work mode
    CMD_PARAM_SET_WORK_MODE     = 0x15,  // Set work mode
    CMD_PARAM_GET_NORMAL_STATE  = 0x16,  // Query always-on/always-off state
    CMD_PARAM_SET_NORMAL_STATE  = 0x17,  // Set always-on/always-off state
    CMD_PARAM_GET_ALL           = 0x20,  // Query all parameters
    CMD_PARAM_SET_ALL           = 0x21,  // Set all parameters 
    CMD_PARAM_PROG_SYNC         = 0x22,
} spi_param_cmd_t;

// slave per channel
#define CHANNELS_PER_SLAVE              4//SLAVE_SPI_MAX_NUM       

// state of quary slave 
typedef enum {
    SLAVE_SYNC_IDLE = 0,            // 空闲状态
    SLAVE_SYNC_WAITING,             // 等待从机应答
    SLAVE_SYNC_COMPLETED,          
    SLAVE_SYNC_STOPTING,
    SLAVE_SYNC_ERROR,
} slave_sync_state_t;

// 从机单个通道参数
typedef struct {
    uint16_t brightness;            // 亮度 (0-255 or 0-999)
    uint16_t strobe_width;          // 频闪脉宽 (0-999)
} slave_channel_param_t;

// 从机参数缓存
typedef struct {
    uint8_t  work_mode;                         // 工作模式 (0:normal, 1:strobe, 2:prog)
    uint8_t  normal_mode_state;                 // 常亮/常灭 (0:L_MODE, 1:H_MODE)
    uint8_t  brightness_range;                  // 亮度范围 (0:255, 1:999)
    uint32_t last_update_tick;                  // 最后更新时间戳
    slave_channel_param_t channel[CHANNELS_PER_SLAVE];  // 4个通道参数
} slave_param_cache_t;

// 从机参数同步上下文
typedef struct {
    slave_sync_state_t      state;              // 同步状态机
    uint8_t                 current_slave_index;// 当前查询的从机索引 (轮询用)
    uint32_t                last_query_tick;    // 上次查询时间
    uint8_t                 retry_count;
    slave_param_cache_t     cache[SLAVE_SPI_MAX_NUM];  // 从机参数缓存 [0-7] (使用SLAVE_SPI_MAX_NUM)
} slave_sync_context_t;


typedef enum {
    ROUTER_FREQ_LOW    = 0,  // >500ms  - 正常频率
    ROUTER_FREQ_MEDIUM = 1,  // 100-500ms - 中等频率
    ROUTER_FREQ_HIGH   = 2   // <100ms  - 高频率
} router_freq_level_t;
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
// 初始化从机参数同步模块
void app_slave_param_sync_init(void);
void app_slave_param_sync_stop(void);
void app_slave_param_sync_start(void);

// 从机参数同步处理函数 (在主循环或定时任务中调用)
void app_slave_param_sync_handle(void);

// 定期批量发送参数修改 (50ms周期调用)
void send_pending_param_changes(void);

// 获取从机参数缓存 (用于显示)
const slave_param_cache_t* get_slave_param_cache(uint8_t slave_id);

// 获取从机通道参数 (用于显示)
const slave_channel_param_t* get_slave_channel_param(uint8_t slave_id, uint8_t channel);

// 设置从机亮度 (通过SPI发送设置命令)
int set_slave_brightness(uint8_t slave_id, uint8_t channel, uint16_t brightness);

// 设置从机频闪脉宽
int set_slave_strobe_width(uint8_t slave_id, uint8_t channel, uint16_t width);

// 设置从机工作模式
int set_slave_work_mode(uint8_t slave_id, uint8_t mode);

// 设置从机常亮/常灭状态
int set_slave_normal_mode_state(uint8_t slave_id, uint8_t state);

// 查询单个从机的所有参数
int query_slave_all_params(uint8_t slave_id);

// 检查从机是否在线
//bool is_slave_online(uint8_t slave_id);

//  智能避让: 通知有网口/串口命令活动 (router调用)
void notify_router_activity(void);

// 暂停/恢复参数同步 (用于空闲超时)
//void pause_slave_param_sync(void);
//void resume_slave_param_sync(void);
//uint8_t is_param_sync_paused(void);
router_freq_level_t get_router_frequency_level(void);

//
int set_slave_cache_brightness(uint8_t slave_id, uint8_t channel, uint16_t brightness);
int set_slave_cache_strobe_width(uint8_t slave_id, uint8_t channel, uint16_t width);
int set_slave_cache_work_mode(uint8_t slave_id, uint8_t mode);
int set_slave_cache_normal_mode_state(uint8_t slave_id, uint8_t state);
//******************************** Declaring ********************************//

#ifdef __cplusplus
}
#endif

#endif /* __APP_SLAVE_PARAM_SYNC_H__ */
