/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_slave_param_sync.c
 *
 * @par dependencies
 * - app_slave_param_sync.h
 * - app_router.h
 * - bsp_spi_bus_dma.h
 *
 * @author ALan | R&D Dept. | CST
 *
 * @brief Slave parameter synchronization implementation
 *
 * Processing flow:
 * 1. Background periodic query (300ms interval)
 * 2. Cache slave parameters locally  
 * 3. Support query and set operations via SPI
 *
 * @version V1.0 2025-10-30     ALan    Initial version
 *          V1.1 2025-11-03     ALan    fix some bugs
 *          V1.2 2025-11-05     ALan    improve spi stability
 *          V1.3 2025-11-11     ALan    add spi abort on repeated failures
 *          V1.4 2025-12-03     ALan    fix set parameter sending issue
 *          V1.5 2025-12-09     ALan    query slave when find slave 
 * @note 
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app_slave_param_sync.h"
#include "app_router.h"
#include "app_master_command.h"
#include "bsp_spi_bus_dma.h"
#include "bsp_timer.h"
#include "rtt_log.h"
#include "protocol_public.h"
#include <string.h>
#include "app_router.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
//static slave_sync_context_t s_sync_ctx        = {0};
static slave_sync_context_t s_sync_ctx          = {
	.state 					= SLAVE_SYNC_IDLE,
	.current_slave_index 	= 0,
	.last_query_tick 		= 0,
	.cache 					= {0},
};

//static uint32_t s_last_set_time                 = 0;    //record last time send set command 
// Parameter modification flags - for batch transmission
static uint8_t s_dirty_flags[SLAVE_SPI_MAX_NUM] = {0};
// Pause flag for idle timeout
//static uint8_t s_param_sync_paused              = 0;

// ? 智能避让: 网口命令活动检测
static uint32_t s_router_activity_ticks[ROUTER_ACTIVITY_HISTORY_SIZE] = {0};
static uint8_t  s_router_activity_index     = 0;  // 循环索引
static uint32_t s_last_router_activity_tick = 0;  // 最后一次活动时间
extern uint8_t  g_spi_error_pending;

static int send_query_all_params_command    (uint8_t slave_id);
static int send_set_all_params_command      (uint8_t slave_id);
// auto yield 
// SPI query and set callback
static void slave_param_query_callback      (uint8_t slave_id, spi_status_t status, 
                                                uint8_t *response_data, uint16_t receive_len);
static void slave_param_set_callback        (uint8_t slave_id, spi_status_t status, 
                                                uint8_t *response_data, uint16_t receive_len);
//******************************** Defines **********************************//

//******************************** Static Functions *************************//
static inline uint8_t bcc_xor(const uint8_t *buf, int len)
{
    uint8_t c = 0;
    for (int i = 0; i < len; i++) c ^= buf[i];
    return c;
}

//******************************** Static Functions *************************//

//******************************** Implementation ***************************//

/**
 * @brief 计算网口命令的频率级别
 * @return router_freq_level_t 频率级别
 * @note 通过最近5次活动的平均间隔判断:
 *       - <100ms:  高频 (实时控制场景)
 *       - 100-500ms: 中频 (定期更新场景)
 *       - >500ms:  low frequency 
 */
router_freq_level_t get_router_frequency_level(void)
{
    uint32_t now = bsp_GetRunTime();
    
    // 检查最近一次活动是否超过2秒 (视为已停止)
    if (bsp_CheckRunTime(s_last_router_activity_tick) > 2000) {
        return ROUTER_FREQ_LOW;  // 恢复正常频率
    }
    
    // 计算最近N次活动的平均间隔
    uint32_t total_interval = 0;
    uint8_t  valid_samples  = 0;
    
    for (uint8_t i = 0; i < ROUTER_ACTIVITY_HISTORY_SIZE - 1; i++) 
    {
        uint8_t curr_idx = (s_router_activity_index + ROUTER_ACTIVITY_HISTORY_SIZE - i) % ROUTER_ACTIVITY_HISTORY_SIZE;
        uint8_t prev_idx = (curr_idx + ROUTER_ACTIVITY_HISTORY_SIZE - 1) % ROUTER_ACTIVITY_HISTORY_SIZE;
        
        if (s_router_activity_ticks[curr_idx] == 0 || s_router_activity_ticks[prev_idx] == 0) {
            break;  
        }
        
        uint32_t interval = s_router_activity_ticks[curr_idx] - s_router_activity_ticks[prev_idx];
        if (interval < 10000) {  
            total_interval += interval;
            valid_samples++;
        }
    }
    if (valid_samples == 0) {
        return ROUTER_FREQ_LOW;  
    }
    uint32_t avg_interval = total_interval / valid_samples;

    if (avg_interval < 100) {
        return ROUTER_FREQ_HIGH;    
    } 
    else if (avg_interval < 500) {
        return ROUTER_FREQ_MEDIUM;  
    } 
    else {
        return ROUTER_FREQ_LOW;     
    }
}

/**
 * @brief 通知参数同步模块有网口/串口命令活动
 * @note  router在转发命令到从机时调用此函数
 *        用于智能避让策略,动态调整查询间隔
 */
void notify_router_activity(void)
{
    uint32_t now = bsp_GetRunTime();
    
    s_last_router_activity_tick = now;
    
    //  update history (circular buffer)
    s_router_activity_index = (s_router_activity_index + 1) % ROUTER_ACTIVITY_HISTORY_SIZE;
    s_router_activity_ticks[s_router_activity_index] = now;
}


/**
 * @brief Initialize slave parameter synchronization module
 */
void app_slave_param_sync_init(void)
{
    memset(&s_sync_ctx, 0, sizeof(s_sync_ctx));
    s_sync_ctx.state                = SLAVE_SYNC_IDLE;
    s_sync_ctx.current_slave_index  = 0;
}



void app_slave_param_sync_stop(void)
{
    s_sync_ctx.state                = SLAVE_SYNC_STOPTING;
    LOG("Param sync paused");
}

void app_slave_param_sync_start(void)
{
    s_sync_ctx.state                = SLAVE_SYNC_IDLE;
    s_sync_ctx.current_slave_index  = 0;
    LOG("Param sync started");
}

__weak void app_slave_param_sync_completed_callback(bool success)
{
    /* NOTE: This function should not be modified, when the callback is needed,
             the app_slave_param_sync_completed_callback could be implemented in the user file. */
    (void)success;
}


/**
 * @brief  Slave parameter sync handler (called in main loop or 10ms timer task)
 * @note   Rotational polling mode: query one slave per interval (like optimized heartbeat)    
 */
void app_slave_param_sync_handle(void)
{
    // Check if paused (for idle timeout)
    //if (s_param_sync_paused) return; 
    if (s_sync_ctx.state == SLAVE_SYNC_COMPLETED || s_sync_ctx.state == SLAVE_SYNC_ERROR) {
        return;
    } 
    
    // Only process in IDLE state
    if (s_sync_ctx.state != SLAVE_SYNC_IDLE) {
        return;  // Waiting for previous query to complete
    }

    uint32_t adaptive_interval          = 3; 
    // Check query interval
    if (bsp_CheckRunTime(s_sync_ctx.last_query_tick) < adaptive_interval) {
        return;
    }

    // Get online slave count   
    uint8_t slave_count = get_found_slave_count();
    if (slave_count == 0) {
        s_sync_ctx.state                = SLAVE_SYNC_COMPLETED;
        s_sync_ctx.last_query_tick      = bsp_GetRunTime();
        app_slave_param_sync_completed_callback(true);
        return; 
    }

    //s_sync_ctx.current_slave_index = s_sync_ctx.current_slave_index + 1;
    if( s_sync_ctx.current_slave_index >= slave_count ) {
        LOG_I("All slaves sync successfully.");
        s_sync_ctx.state                = SLAVE_SYNC_COMPLETED;
        app_slave_param_sync_completed_callback(true); 
        return;
    }

    uint8_t slave_id                    = s_sync_ctx.current_slave_index + 1; // Convert to 1-based ID

    if (is_slave_online_by_id(slave_id)) {     
        s_sync_ctx.retry_count          = 0;
        if (send_query_all_params_command(slave_id) == 0) { //send success
            s_sync_ctx.state            = SLAVE_SYNC_WAITING;
            s_sync_ctx.last_query_tick  = bsp_GetRunTime();
        }
        else {
            slave_param_query_callback(s_sync_ctx.current_slave_index, SPI_STATUS_ERROR, NULL, 0);
        }
    }
    else {  
        LOG_W("Slave %d is not online, skipping sync.", slave_id);
        s_sync_ctx.current_slave_index++;
        s_sync_ctx.last_query_tick       = bsp_GetRunTime(); 
    }
}

/**
 * @brief   Send query all parameters command
 * @param   slave_id Slave ID (1-8)
 * @return  0: success, -1: failed
 */
static int send_query_all_params_command(uint8_t slave_id)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return -1;
    }
    uint8_t slave_index0 = slave_id - 1;
    // Check SPI bus status
    if (bsp_spi_get_status(slave_index0) != SPI_STATUS_OK) {
        return -1;
    }

    if (bsp_spi_is_bus_locked()) {
        return -1;  
    }

    // Construct query frame
    uint8_t frame[SPI_FORWARD_FRAME_TOTAL]  = {0};
    spi_payload_t *payload                  = (spi_payload_t *)&frame[0];
    spi_function_t *func_payload            = (spi_function_t *)payload->reserved;

    payload->frame_head                     = SPI_FRAME_HEAD;
    payload->frame_type                     = FRAME_TYPE_CMD;
    payload->protocol_version               = SPI_COMM_VERSION;
    payload->slave_id                       = slave_id;
    
    func_payload->function                  = FUNC_EXPAND;
    func_payload->command                   = CMD_PARAM_GET_ALL;
    func_payload->data1                     = 0;
    func_payload->data2                     = 0;

    payload->bcc                            = bcc_xor((uint8_t*)payload, 
                                                sizeof(spi_payload_t) - 1);

    spi_status_t result = bsp_spi_comm(
        slave_index0, 
        (uint8_t *)payload, 
        sizeof(spi_payload_t), 
        SPI_MASTER_DEFAULT_RESP_LEN,
        SPI_MASTER_COMM_DELAY_MS,
        SPI_MASTER_COMM_TIMEOUT_MS,
        slave_param_query_callback
    );
    if (result != SPI_STATUS_OK) {
        LOG_W("Failed to query slave %d, reason is%d", slave_id,result);
        return -1;
    }
	LOG_I("Successful query slave%d!",bsp_GetRunTime());
    return 0;
}

/**
 * @brief  Query callback function
 * @note   Simplified: no retry logic, just parse data and return to IDLE
 */
static void slave_param_query_callback(uint8_t slave_id, spi_status_t status, 
                                       uint8_t *response_data, uint16_t receive_len)
{
    uint8_t actual_slave_id     = slave_id + 1; // Convert to 1-based
    spi_payload_t *payload      = (spi_payload_t *)(response_data);

    // Verify state
    if ( s_sync_ctx.current_slave_index != slave_id || 
        (s_sync_ctx.state != SLAVE_SYNC_WAITING  && status == SPI_STATUS_OK) ) {
        LOG_W("[Sync] Unexpected callback for slave %d in state %d", 
                    actual_slave_id, s_sync_ctx.state);
        return;
    }

    bool success = false;
    if (status == SPI_STATUS_OK && response_data != NULL && receive_len >0) 
    {
        spi_payload_t *reply = (spi_payload_t *)response_data;

        if (payload->bcc != bcc_xor((uint8_t*)response_data, receive_len - 1)) {
            LOG_E("[Query callback]Invalid SPI reply ,invalid bcc (calc on %d bytes)", receive_len - 1);
        }
        else if (   reply->frame_head != SPI_FRAME_HEAD ||
                    reply->frame_type != FRAME_TYPE_ACK ||
                    reply->slave_id   != actual_slave_id) {

            LOG_W("[Sync] Invalid frame format from slave %d (head=0x%02X, type=0x%02X, id=%d)",
                    actual_slave_id, reply->frame_head, reply->frame_type, reply->slave_id);
        }
        else{
            // Parse parameters and update cache
            // reserved[21]: [func][cmd][4×brightness(8B)][4×strobe_width(8B)][mode][h_l][range]
            uint8_t *data                       = reply->reserved;
            slave_param_cache_t *cache          = &s_sync_ctx.cache[slave_id];
            cache->last_update_tick             = bsp_GetRunTime();
            // Parse 4-channel brightness (little-endian)
            for (int ch = 0; ch < CHANNELS_PER_SLAVE; ch++) {
                cache->channel[ch].brightness   = data[2 + ch * 2] | (data[2 + ch * 2 + 1] << 8);
            }
            // Parse 4-channel strobe width (little-endian)
            for (int ch = 0; ch < CHANNELS_PER_SLAVE; ch++) {
                cache->channel[ch].strobe_width = data[10 + ch * 2] | (data[10 + ch * 2 + 1] << 8);
            }
            cache->work_mode                    = data[18];     // Work mode (0-2)
            cache->normal_mode_state            = data[19];     // H/L state (0-1)
            cache->brightness_range             = data[20];     // Brightness range (0-1)
            #if DEBUG_MODE
                LOG("Slave %d param sync OK,time:%d ", actual_slave_id,bsp_GetRunTime());
            #endif
            success = true;
        } 
    } 
    else {
        LOG_W("[Sync] Communication error with slave %d, status=%d", actual_slave_id, status);
    }

    if (success) {
        s_sync_ctx.current_slave_index++;
        s_sync_ctx.retry_count = 0;
    }
    else {
        s_sync_ctx.retry_count++;
        LOG_W("Slave %d sync failed. Retry count: %d/%d", actual_slave_id, s_sync_ctx.retry_count, SLAVE_PARAM_SYNC_RETRIES_MAX);

        if (s_sync_ctx.retry_count >= SLAVE_PARAM_SYNC_RETRIES_MAX) {
            LOG_E("[Sync] Slave %d sync failed after %d retries, aborting.", 
                    actual_slave_id, SLAVE_PARAM_SYNC_RETRIES_MAX);
            s_sync_ctx.state = SLAVE_SYNC_ERROR;
            app_slave_param_sync_completed_callback(false); // 触发失败回调
            set_light_alarm(STATUS_SPI_COMM_ERROR);
            return;
        }
    }

    s_sync_ctx.state = SLAVE_SYNC_IDLE;
    s_sync_ctx.last_query_tick = bsp_GetRunTime();
}


/************************************************************* interface *************************************************************/
/**
 * @brief   Get slave parameter cache
 * @param   Slave ID (1-8)
 * @return  Parameter cache pointer, NULL on failure
 */
const slave_param_cache_t* get_slave_param_cache(uint8_t slave_id)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return NULL;
    }
    return &s_sync_ctx.cache[slave_id - 1];
}

/**
 * @brief   Get slave channel parameter
 * @param   Slave ID (1-8)
 * @param   Channel number (0-3)
 * @return  Channel parameter pointer, NULL on failure
 */
const slave_channel_param_t* get_slave_channel_param(uint8_t slave_id, uint8_t channel)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM || channel >= CHANNELS_PER_SLAVE) {
        return NULL;
    }
    return &s_sync_ctx.cache[slave_id - 1].channel[channel];
}

/**
 * @brief   Set slave brightness (only update cache and flag, batch send periodically)
 * @param slave_id      (1-8)
 * @param channel       (0-3)
 * @param brightness    (0-255 or 0-999)
 * @return              0: success, -1: failed
 */
int set_slave_brightness(uint8_t slave_id, uint8_t channel, uint16_t brightness)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM || channel >= CHANNELS_PER_SLAVE) {
        return -1;
    }

    uint8_t s_brightness_range = get_light_brightness_range(); // 0:0-255, 1:0-999
    if (s_brightness_range == 0){
        if (brightness > 255)
            brightness = 255;
    }
    else{
         if (brightness > 999)
            brightness = 999;
    }
    //  Update local cache
    s_sync_ctx.cache[slave_id - 1].channel[channel].brightness = brightness;
    //  Mark this slave parameter as modified
    s_dirty_flags[slave_id - 1] = 1;
    
    return 0;
}
/**
 * @brief   Set slave brightness (only update cache , don't batch send)
 */
int set_slave_cache_brightness(uint8_t slave_id, uint8_t channel, uint16_t brightness)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM || channel >= CHANNELS_PER_SLAVE) {
        return -1;
    }

    uint8_t s_brightness_range = get_light_brightness_range(); // 0:0-255, 1:0-999
    if (s_brightness_range == 0){
        if (brightness > 255)
            brightness = 255;
    }
    else{
         if (brightness > 999)
            brightness = 999;
    }

    s_sync_ctx.cache[slave_id - 1].channel[channel].brightness = brightness;
    return 0;
}

/**
 * @brief Set slave strobe width (only update cache and flag, batch send periodically)
 * @param slave_id (1-8)
 * @param channel  (0-3)
 * @param width    (0-999)
 * @return         0: success, -1: failed
 */
int set_slave_strobe_width(uint8_t slave_id, uint8_t channel, uint16_t width)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM || channel >= CHANNELS_PER_SLAVE) {
        return -1;
    }
    if (width > 999) {
        width = 999; 
    }

    //  Update local cache
    s_sync_ctx.cache[slave_id - 1].channel[channel].strobe_width = width;
    //  Mark this slave parameter as modified
    s_dirty_flags[slave_id - 1] = 1;
    return 0;
}

int set_slave_cache_strobe_width(uint8_t slave_id, uint8_t channel, uint16_t width)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM || channel >= CHANNELS_PER_SLAVE) {
        return -1;
    }

    if (width > 999) {
        width = 999; 
    }

    // Update local cache, but no send command to slave
    s_sync_ctx.cache[slave_id - 1].channel[channel].strobe_width = width;
    return 0;
}

/**
 * @brief Set slave work mode (only update cache and flag, batch send periodically)
 * @param slave_id Slave ID (1-8)
 * @param mode Work mode (0:normal, 1:strobe, 2:prog)
 * @return 0: success, -1: failed
 */
int set_slave_work_mode(uint8_t slave_id, uint8_t mode)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return -1;
    }

    // Update local cache
    s_sync_ctx.cache[slave_id - 1].work_mode = mode;
    
    // Mark this slave parameter as modified
    s_dirty_flags[slave_id - 1] = 1;
    
    return 0;
}

int set_slave_cache_work_mode(uint8_t slave_id, uint8_t mode)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return -1;
    }

    // Update local cache
    s_sync_ctx.cache[slave_id - 1].work_mode = mode; 
    return 0;
}

/**
 * @brief 设置从机常亮/常灭状态 (仅更新缓存和标记,定期批量发送) / Set slave always-on/always-off state (only update cache and flag, batch send periodically)
 * @param slave_id Slave ID (1-8)
 * @param state    State (0:L_MODE, 1:H_MODE)
 * @return         0: success, -1: failed
 */
int set_slave_normal_mode_state(uint8_t slave_id, uint8_t state)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return -1;
    }

    // 更新本地缓存 / Update local cache
    s_sync_ctx.cache[slave_id - 1].normal_mode_state = state;
    
    // 标记该从机参数已修改 / Mark this slave parameter as modified
    s_dirty_flags[slave_id - 1] = 1;
    
    return 0;
}

int set_slave_cache_normal_mode_state(uint8_t slave_id, uint8_t state)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return -1;
    }

    // 更新本地缓存 / Update local cache
    s_sync_ctx.cache[slave_id - 1].normal_mode_state = state;
     
    return 0;
}

/************************************************************* set *************************************************************/


/**
 * @brief Set command callback
 */
static void slave_param_set_callback(uint8_t slave_id, spi_status_t status, 
                                     uint8_t *response_data, uint16_t receive_len)
{
    uint8_t actual_slave_id = slave_id + 1;

    if (status == SPI_STATUS_OK && response_data != NULL) {
        spi_payload_t *reply = (spi_payload_t *)response_data;
        
        if (reply->frame_head == SPI_FRAME_HEAD && 
            reply->frame_type == FRAME_TYPE_ACK &&
            reply->slave_id == actual_slave_id) {
            LOG("Slave %d param set OK", actual_slave_id);
                        //V1.2 improvement
            bsp_spi_reset_consecutive_reinit_fail_count(); // reset fail count on success
        } else {
            LOG_W("Slave %d param set invalid reply", actual_slave_id);
        }
    } else {
        LOG_W("Slave %d param set error", actual_slave_id);
    }
}

/**
 * @brief   Query all parameters of a single slave
 * @param   slave_id Slave ID (1-8)
 * @return  0: success, -1: failed
 */
int query_slave_all_params(uint8_t slave_id)
{
    return send_query_all_params_command(slave_id);
}

/**
 * @brief   Check if slave is online
 * @param   slave_id ID (1-8)
 * @return  true: online, false: offline
 * @note    Directly call app_master_command's status check to avoid redundant online status maintenance
 */
/*bool is_slave_online(uint8_t slave_id)
{
    return is_slave_online_by_id(slave_id);
}*/

/**
 * @brief Send set all parameters command
 * @param slave_id Slave ID (1-8)
 * @return 0: success, -1: failed
 */
static int send_set_all_params_command(uint8_t slave_id)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return -1;
    }
    uint8_t slave_index0 = slave_id - 1;

    if (bsp_spi_get_status(slave_index0) != SPI_STATUS_OK) {
        return -1;
    }

    uint8_t frame[SPI_FORWARD_FRAME_TOTAL] = {0};
    spi_payload_t *payload          = (spi_payload_t *)&frame[0];
    spi_function_t *func_payload    = (spi_function_t *)payload->reserved;

    payload->frame_head             = SPI_FRAME_HEAD;
    payload->frame_type             = FRAME_TYPE_CMD;
    payload->protocol_version       = SPI_COMM_VERSION;
    payload->slave_id               = slave_id;
    func_payload->function          = FUNC_EXPAND;
    func_payload->command           = CMD_PARAM_SET_ALL;  
    // Get cache data
    slave_param_cache_t *cache      = &s_sync_ctx.cache[slave_index0];
    
    // Construct format data (21-byte )
    // [0-1]: function + command
    // [2-9]: 4ch brightness × uint16_t (8B)
    // [10-17]: 4ch strobe_width × uint16_t (8B)
    // [18-20]: mode + H/L + range (3B)
    
    uint8_t *data = payload->reserved;
    // [0-1] already set by func_payload
    
    // [2-9] 4-channel brightness (little-endian)
    for (int ch = 0; ch < CHANNELS_PER_SLAVE; ch++) {
        data[2 + ch * 2] = cache->channel[ch].brightness & 0xFF;
        data[2 + ch * 2 + 1] = (cache->channel[ch].brightness >> 8) & 0xFF;
    }
    
    // [10-17] 4-channel strobe width (little-endian)
    for (int ch = 0; ch < CHANNELS_PER_SLAVE; ch++) {
        data[10 + ch * 2] = cache->channel[ch].strobe_width & 0xFF;
        data[10 + ch * 2 + 1] = (cache->channel[ch].strobe_width >> 8) & 0xFF;
    }
    
    // [18-20] Mode and status
    data[18] = cache->work_mode;
    //data[19] = cache->normal_mode_state;
	//data[18] = get_light_mode();
    data[19] = get_light_normal_mode_state();  // always use global setting
    data[20] = cache->brightness_range;  // range reserved


    payload->bcc = bcc_xor((uint8_t*)payload, sizeof(spi_payload_t) - 1);

    spi_status_t result = bsp_spi_comm(
        slave_index0, 
        (uint8_t *)payload, 
        sizeof(spi_payload_t), 
        SPI_MASTER_DEFAULT_RESP_LEN,
        SPI_MASTER_COMM_DELAY_MS,
        SPI_MASTER_COMM_TIMEOUT_MS,
        slave_param_set_callback
    );

    if (result != SPI_STATUS_OK) {
        LOG_E("Failed to set params for slave %d", slave_id);
        return -1;
    }

    //s_last_set_time = bsp_GetRunTime();
    
    LOG("Successful Set params for slave %d", slave_id);
    return 0;
}

/**
 * @brief Periodically batch send parameter changes (called every 50ms)
 * @note  Check dirty flags and batch send modified slave parameters
 */
void send_pending_param_changes(void)
{
    // if spi bus is busy, wait for next time
    if (bsp_spi_is_bus_locked()) {
        return;
    }

    for (uint8_t i = 0; i < get_found_slave_count(); i++) 
    {
        if (s_dirty_flags[i]) 
        {
            uint8_t slave_id = i + 1;
            // Check if slave is online
            if (is_slave_online_by_id(slave_id)) 
            {
                // send all parameters of this slave
                if (send_set_all_params_command(slave_id) == 0)
                {
                    // Clear dirty flag only on successful send
                    s_dirty_flags[i] = 0;
                    // Break to ensure only one slave is processed per call
                    break; 
                }
            }
            else
            {
                // if slave is offline, clear the flag
                s_dirty_flags[i] = 0;
            }
        }
    }
}





//******************************** Implementation ***************************//
