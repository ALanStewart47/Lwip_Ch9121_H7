/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_master_command.c
 *
 * @par dependencies
 * - app_master_command.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief master send command to slave, control or acquire slave
 *
 * Processing flow:
 * call directly.
 *
 * @version     V1.0    2025-10-22   ALan      add find slave function
 *              V1.1    2025-10-28   ALan      add heartbeat function
 *              V1.2    2025-12-12   ALan      add slave type and sync command
 * @note 
 *        master send command to slave, control or acquire slave 
 *       
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app_master_command.h"
#include "app_slave_param_sync.h"
#include "bsp_spi_bus_dma.h"
#include "app_router.h"
#include "bsp_timer.h"
#include "rtt_log.h"
#include <string.h>
//******************************** Includes *********************************//

//******************************** Defines **********************************//
static machine_type_t           g_machine_type[SLAVE_SPI_MAX_NUM]   = {DIGIT_MACHINE} ;             

///////find slave 
volatile bool                   g_start_find_slave_scan             = false;
volatile uint8_t                g_found_slave_count                 = 0;
volatile scan_result_t          g_scan_result                       = SCAN_RESULT_NONE;
static master_scan_state_t      s_scan_state                        = SCAN_IDLE;
static uint8_t                  s_current_scan_slave_id             = 0; 
static uint32_t                 s_scan_start_time                   = 0;
static uint8_t                  s_retry_count                       = 0;
static uint32_t                 s_retry_start_time                  = 0; 
///////find slave 

///////heartbeat
static heartbeat_state_t        s_heartbeat_state                    = HEARTBEAT_IDLE;
static uint32_t                 s_heartbeat_last_tick                = 0;
static uint8_t                  s_heartbeat_current_slave            = 0;    
static uint8_t                  s_heartbeat_slave_count              = 0;    
static slave_heartbeat_info_t   s_slave_heartbeat[SLAVE_SPI_MAX_NUM] = {0};
static bool                     s_heartbeat_paused                   = false;                    
///////heartbeat

///////prog sync

///////prog sync

///////find slave 
static spi_master_status_t send_find_command(uint8_t slave_id);
static void master_cmd_spi_found_slave_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len);
static void master_cmd_spi_heartbeat_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len);
///////find slave 

///////heartbeat 
static spi_master_status_t send_heartbeat_command(uint8_t slave_id);
static void process_heartbeat_error(uint8_t slave_id);
static void report_slave_error(uint8_t slave_id);
///////heartbeat 

///////prog sync 
static spi_master_status_t send_prog_slave_command(uint8_t slave_id);
static void master_cmd_prog_sync_slave_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len);
static void prog_sync_process(void);
///////prog sync 
//******************************** Defines **********************************//

//******************************** Declaring ********************************//

static inline uint8_t bcc_xor(const uint8_t *buf, int len)
{
    uint8_t c = 0;
    for (int i = 0; i < len; i++) c ^= buf[i];
    return c;
}

void app_master_command_param_init(void)
{
    g_start_find_slave_scan     = false;
    g_found_slave_count         = 0;
    g_scan_result               = SCAN_RESULT_NONE;
    
    s_scan_state                = SCAN_IDLE;
    s_current_scan_slave_id     = 0; 
    s_retry_count               = 0;
    s_retry_start_time          = 0;

    s_heartbeat_state           = HEARTBEAT_IDLE;
    s_heartbeat_last_tick       = 0;
    s_heartbeat_current_slave   = 0;
    s_heartbeat_slave_count     = 0;
}

/****************************************************************************************************/
/* scan slave																						*/ 
/****************************************************************************************************/
scan_result_t get_scan_result(void)
{
    return g_scan_result;
}

void clear_scan_result(void)
{
    g_scan_result = SCAN_RESULT_NONE;
}

uint8_t get_found_slave_count(void)
{
    return g_found_slave_count;
}

spi_master_status_t master_find_slave_ctrl(bool on_off)
{
    if (g_start_find_slave_scan == on_off){
        return SPI_MASTER_ERROR;  
    }
    g_start_find_slave_scan = on_off;
    return SPI_MASTER_OK;
}

master_scan_state_t get_scan_state(void)
{
    return s_scan_state;
}

static void master_find_slave_process(void)
{
    switch (s_scan_state)
    {
    case SCAN_IDLE:
        if(g_start_find_slave_scan){
            s_scan_state    = SCAN_STARTING;
            //g_scan_result   = SCAN_RESULT_NONE;
        }
        break;

    case SCAN_STARTING:
        LOG("start scan slave:%d",bsp_GetRunTime());
        g_found_slave_count     = 0;
        s_current_scan_slave_id = 1;
        s_retry_count           = 0;
        s_scan_start_time       = bsp_GetRunTime();
        // Wait a bit for any pending SPI operations to complete
        if (bsp_spi_is_bus_locked()) {
            if (bsp_CheckRunTime(s_scan_start_time) < 50) {
                // Wait for bus to be free, max 50ms
                break;
            }
            LOG_W("SPI bus still locked after 50ms, proceeding anyway");
        }
        if (send_find_command(s_current_scan_slave_id) == SPI_MASTER_OK){
            s_scan_state = SCAN_WAITING_REPLY;
        }
        else{
            s_retry_count++;
            if (s_retry_count >= RETRY_MAX_COUNT){
                //g_scan_result = SCAN_RESULT_STOPPED;
                s_scan_state = SCAN_STOPPED;
                LOG_E("max retry find slave ,stop scan");
            }
            else{
                s_retry_start_time = bsp_GetRunTime();
                s_scan_state = SCAN_RETRY_DELAY;
                LOG_W("Retry %d for slave %d", s_retry_count, s_current_scan_slave_id);
            }
        }
        break;

    case SCAN_WAITING_REPLY:
        // if send_find_command succeeded, waiting for callback to change state
        // this case will handle timeout if no response received
        if (bsp_CheckRunTime(s_scan_start_time) > FIND_SLAVE_TIMEOUT_MS){
            s_retry_count++;
            if (s_retry_count >= RETRY_MAX_COUNT){
                //g_scan_result   = SCAN_RESULT_STOPPED;
                s_scan_state    = SCAN_STOPPED;
                LOG_E("Max retry count reached for slave %d after timeout", s_current_scan_slave_id);
            }
            else{
                s_retry_start_time = bsp_GetRunTime();
                s_scan_state = SCAN_RETRY_DELAY;
                LOG_W("Timeout retry %d for slave %d", s_retry_count, s_current_scan_slave_id);
            }
        }
        break;

    case SCAN_RETRY_DELAY:
        if (bsp_CheckRunTime(s_retry_start_time) >= RETRY_DELAY_MS){
            if (send_find_command(s_current_scan_slave_id) == SPI_MASTER_OK){
                s_scan_state = SCAN_WAITING_REPLY;
            }
            else{
                s_retry_count++;
                if (s_retry_count >= RETRY_MAX_COUNT){
                    //g_scan_result = SCAN_RESULT_STOPPED;
                    s_scan_state  = SCAN_STOPPED;
                    LOG_E("Max retry count reached for slave %d", s_current_scan_slave_id);
                }
                else{
                    s_retry_start_time = bsp_GetRunTime();
                    LOG_W("Retry %d for slave %d", s_retry_count, s_current_scan_slave_id);
                }
            }
        }
        break;

    case SCAN_NEXT_SLAVE:
        s_current_scan_slave_id++;
        s_retry_count = 0;
        if (s_current_scan_slave_id > SLAVE_SPI_MAX_NUM){
            g_scan_result   = SCAN_RESULT_SUCCESS;
            s_scan_state    = SCAN_STOPPED;
            LOG_I("Scan finished. Found %d slaves", g_found_slave_count);
        }
        else{
            if (send_find_command(s_current_scan_slave_id) == SPI_MASTER_OK){
                s_scan_state = SCAN_WAITING_REPLY;
            }
            else{
                s_retry_start_time  = bsp_GetRunTime();
                s_scan_state        = SCAN_RETRY_DELAY;
                s_retry_count       = 1;
                LOG_W("Failed to send to slave %d, entering retry", s_current_scan_slave_id);
            }
        }
        break;

    case SCAN_STOPPED:
        g_start_find_slave_scan = false;
        s_scan_state            = SCAN_IDLE;
        g_scan_result           = SCAN_RESULT_STOPPED;
        s_retry_count           = 0;  // Reset retry count
        s_current_scan_slave_id = 0;  // Reset slave id
        for (uint8_t slave_id = 0; slave_id < g_found_slave_count; slave_id++) {
            s_slave_heartbeat[slave_id].status            = SLAVE_STATUS_ONLINE;
        }
        app_slave_param_sync_start();
	LOG("scan stopped! found %d slaves,time:%d", g_found_slave_count,bsp_GetRunTime());
        break;
    default:
        break;
    }
}


static spi_master_status_t send_find_command(uint8_t slave_id)
{
    if(slave_id > SLAVE_SPI_MAX_NUM)
        return SPI_MASTER_ERRORPARAMETER;
    
    // ? 提前检查: SPI总线是否被锁定
    if (bsp_spi_is_bus_locked()) {
        return SPI_MASTER_ERROR;
    }
    
    uint8_t slave_index0 = slave_id - 1;

    if (bsp_spi_get_status(slave_index0) != SPI_STATUS_OK) {
        return SPI_MASTER_ERROR; 
    }

    uint8_t frame[SPI_FORWARD_FRAME_TOTAL] = {0};
    spi_payload_t *payload = (spi_payload_t *)&frame[0];
    spi_function_t *func_payload = (spi_function_t *)payload->reserved;

    payload->frame_head         = SPI_FRAME_HEAD;
    payload->frame_type         = FRAME_TYPE_CMD;
    payload->protocol_version   = SPI_COMM_VERSION;
    payload->slave_id           = slave_id;

    func_payload->function      = FUNC_SYSTEM_INFO;
    func_payload->command       = CMD_SYS_FIND_SLAVE;
    func_payload->data1         = 0;
    func_payload->data2         = 0;

    payload->bcc = bcc_xor((uint8_t*)payload, sizeof(spi_payload_t) - 1);

    spi_status_t result = bsp_spi_comm( slave_index0, 
                                        (uint8_t *)payload, 
                                        sizeof(spi_payload_t), 
                                        SPI_MASTER_DEFAULT_RESP_LEN,
                                        SPI_MASTER_FIND_SLAVE_MS,
                                        SPI_MASTER_FIND_SLAVE_TIMEOUT,
                                        master_cmd_spi_found_slave_callback
    );                   

    if (result != SPI_STATUS_OK){
        LOG_E("Error:can not send find command ");
        return SPI_MASTER_ERROR;
    }
    LOG("Sent find command to slave %d", slave_id);
    s_scan_start_time = bsp_GetRunTime();
    return SPI_MASTER_OK;
}

static void master_cmd_spi_found_slave_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len)
{
    // 确保回调对应的是当前正在等待的从机
    if (s_scan_state != SCAN_WAITING_REPLY || slave_id != (s_current_scan_slave_id - 1)) {
        LOG_E ("Unexpected callback for slave %d in state %d", slave_id + 1, s_scan_state);
        return;
    }

    uint8_t *data = response_data;

    if (status == SPI_STATUS_OK && response_data != NULL) {
        spi_payload_t *reply = (spi_payload_t *)response_data;
        
        if (reply->frame_head == SPI_FRAME_HEAD && 
            reply->frame_type == FRAME_TYPE_ACK &&
            reply->slave_id == s_current_scan_slave_id) {
            
            uint8_t *rece_data = reply->reserved;
            
            LOG_I("Slave %d found", s_current_scan_slave_id);
            g_found_slave_count++;
            s_scan_state                = SCAN_NEXT_SLAVE;
            machine_type_t s_type       = DIGIT_MACHINE;
            s_type                      = (machine_type_t)(rece_data[2]); 
            g_machine_type[slave_id]    = s_type;
            LOG_I ("Slave is 0x%02X type", g_machine_type[slave_id]);
        } 
        else {
            // 响应解析失败,尝试重发
            s_retry_count++;
            if(s_retry_count >= RETRY_MAX_COUNT) {
                LOG_E("Max retry count reached for slave %d after invalid response", s_current_scan_slave_id);
                s_scan_state = SCAN_STOPPED;
            } else {
                s_scan_start_time   = bsp_GetRunTime();
                s_scan_state        = SCAN_RETRY_DELAY;
                LOG_W("Invalid response retry %d for slave %d", s_retry_count, s_current_scan_slave_id);
                LOG_E("Slave received frame with invalid header : 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",
                        data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);
            }
        }
    } 
    else {
        // 通信失败,尝试重发
        s_retry_count++;
        if(s_retry_count >= RETRY_MAX_COUNT) {
            LOG_E("Max retry count reached for slave %d after comm error", s_current_scan_slave_id);
            s_scan_state = SCAN_STOPPED;
        } else {
            s_scan_start_time   = bsp_GetRunTime();
            s_scan_state        = SCAN_RETRY_DELAY;
            LOG_W("Comm error retry %d for slave %d", s_retry_count, s_current_scan_slave_id);
        }
    }
}

/****************************************************************************************************/
/* Hearbeat function																				*/
/****************************************************************************************************/

static void process_heartbeat_error(uint8_t slave_id)
{
    s_slave_heartbeat[slave_id].error_count++;
    
    if (s_slave_heartbeat[slave_id].error_count >= HEARTBEAT_ERROR_THRESHOLD) {

        uint8_t current_slave_count = get_found_slave_count();
        uint8_t actual_slave_id     = slave_id + 1;
        
        if (actual_slave_id <= current_slave_count) {
            s_slave_heartbeat[slave_id].status = SLAVE_STATUS_ERROR;
            report_slave_error(slave_id);
            LOG_E("Slave %d detected as ERROR", actual_slave_id);
        } 
        else {
            s_slave_heartbeat[slave_id].status      = SLAVE_STATUS_UNKNOWN;
            s_slave_heartbeat[slave_id].error_count = 0;
            LOG_I("Slave %d out of range (count reduced), cleared error", actual_slave_id);
        }
    } else {
        LOG_W("Slave %d heartbeat error count: %d/%d", 
              slave_id + 1, 
              s_slave_heartbeat[slave_id].error_count,
              HEARTBEAT_ERROR_THRESHOLD);
    }
}

static void report_slave_error(uint8_t slave_id)
{
    set_light_alarm(STATUS_SPI_COMM_ERROR);
    LOG_E("!!! SLAVE %d FAULT REPORTED !!!", slave_id + 1);
}

void app_master_heartbeat_pause(void)
{
    s_heartbeat_paused      = true;
}

void app_master_heartbeat_resume(void)
{
    s_heartbeat_paused      = false;
    s_heartbeat_last_tick   = bsp_GetRunTime();
}

/**
 * @brief   heartbeat process 
 * @param   None
 * @return  None
 * @note   
 */
static void master_heartbeat_process(void)
{
    if (s_heartbeat_paused) {
        return; 
    }
    switch (s_heartbeat_state)
    {
	case HEARTBEAT_IDLE:
	{
		router_freq_level_t level           = get_router_frequency_level();
		uint32_t heartbeat_interval         = 0;
    
		switch (level) {
		case ROUTER_FREQ_HIGH:
			return ;                                    //  直接跳过,不查询
		case ROUTER_FREQ_MEDIUM:
			heartbeat_interval              = 20000;                 //  20s
		case ROUTER_FREQ_LOW:
		default:
			heartbeat_interval              = HEARTBEAT_INTERVAL_MS;  // 10s
		}
	
        if (bsp_CheckRunTime(s_heartbeat_last_tick) >= heartbeat_interval) {
            uint8_t current_count           = get_found_slave_count();
            if (current_count == 0) {
                s_heartbeat_last_tick       = bsp_GetRunTime();
                return;
            }
            
            // 轮询模式: 只查询一个从机
            s_heartbeat_slave_count         = current_count;
            s_heartbeat_current_slave++;  // 移动到下一个从机
            if (s_heartbeat_current_slave > s_heartbeat_slave_count) {
                s_heartbeat_current_slave   = 1;  // 循环回到第一个从机
            }
            s_heartbeat_state = HEARTBEAT_SENDING;
        }
	}
	break;

    case HEARTBEAT_SENDING:
        if (send_heartbeat_command(s_heartbeat_current_slave) == SPI_MASTER_OK) {
            s_heartbeat_state               = HEARTBEAT_WAITING;
            s_heartbeat_last_tick           = bsp_GetRunTime();
        } 
        else { 
            // BUSY时回到IDLE，重新计时等待发送
			s_heartbeat_state               = HEARTBEAT_IDLE;
            s_heartbeat_last_tick           = bsp_GetRunTime();
        }
        break;

    case HEARTBEAT_WAITING:
        // 超时保护
        if (bsp_CheckRunTime(s_heartbeat_last_tick) > (SPI_MASTER_COMM_TIMEOUT_MS + 5)) {
            LOG_E("Heartbeat timeout for slave %d", s_heartbeat_current_slave);
            process_heartbeat_error(s_heartbeat_current_slave - 1);
            
            // ? 超时后直接回到IDLE,等待下次间隔(查询下一个从机)
            s_heartbeat_state               = HEARTBEAT_IDLE;
            s_heartbeat_last_tick           = bsp_GetRunTime();
        }
        break;
    default:
        s_heartbeat_state                   = HEARTBEAT_IDLE;
        break;
    }
}


/**
 * @brief:  send heartbeat command to slave
 * @param:  slave_id : 1..8
 * @return: spi_master_status_t
 * @note   
 */
static spi_master_status_t send_heartbeat_command(uint8_t slave_id)
{
    if (slave_id > SLAVE_SPI_MAX_NUM)
        return SPI_MASTER_ERRORPARAMETER;
    
    if (bsp_spi_is_bus_locked()) {
         LOG_W("[heartbeat]SPI bus locked, skip packing"); 
        return SPI_MASTER_ERROR;
    }
    
    uint8_t slave_index0 = slave_id - 1;
    if (bsp_spi_get_status(slave_index0) != SPI_STATUS_OK) {
         LOG_W("[heartbeat]Slave %d is busy", slave_id); 
        return SPI_MASTER_ERROR; 
    }

    // make heartbeat frame 
    uint8_t frame[SPI_FORWARD_FRAME_TOTAL]  = {0};

    spi_payload_t *payload       = (spi_payload_t *)&frame[0];
    spi_function_t *func_payload = (spi_function_t *)payload->reserved;
   
    payload->frame_head          = SPI_FRAME_HEAD;
    payload->frame_type          = FRAME_TYPE_CMD;
    payload->protocol_version    = SPI_COMM_VERSION;
    payload->slave_id            = slave_id;
    func_payload->function       = FUNC_SYSTEM_INFO;
    func_payload->command        = CMD_SYS_HEARTBEAT;
    func_payload->data1          = 0;
    func_payload->data2          = 0;

    payload->bcc = bcc_xor((uint8_t*)payload, sizeof(spi_payload_t) - 1);

    spi_status_t result = bsp_spi_comm(
        slave_index0, 
        (uint8_t *)payload, 
        sizeof(spi_payload_t), 
        SPI_MASTER_DEFAULT_RESP_LEN,
        SPI_MASTER_COMM_DELAY_MS,
        SPI_MASTER_COMM_TIMEOUT_MS,
        master_cmd_spi_heartbeat_callback
    );

    if (result != SPI_STATUS_OK) {
         LOG_E("Failed to send heartbeat to slave %d, status: %d", slave_id, result); 
        return SPI_MASTER_ERROR;
    }
    return SPI_MASTER_OK;
}

/**
 * @brief:  SPI heartbeat callback
 * @param:  slave_id : 0..7
 *          status : receive status
 *          response_data : received data pointer
 *          receive_len : received data length
 * @return: None
 */
static void master_cmd_spi_heartbeat_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len)
{
    uint8_t actual_slave_id = slave_id + 1; 
    
    if (s_heartbeat_state != HEARTBEAT_WAITING || actual_slave_id != s_heartbeat_current_slave) {
        LOG_W("[heartbeat]Unexpected callback for slave %d", actual_slave_id);
        return;
    }

    if (status == SPI_STATUS_OK && response_data != NULL) {
        spi_payload_t *reply = (spi_payload_t *)response_data;
        if (reply->frame_head == SPI_FRAME_HEAD && 
            reply->frame_type == FRAME_TYPE_ACK &&
            reply->slave_id == actual_slave_id) {
            
            s_slave_heartbeat[slave_id].status            = SLAVE_STATUS_ONLINE;
            s_slave_heartbeat[slave_id].error_count       = 0;
            s_slave_heartbeat[slave_id].last_success_tick = bsp_GetRunTime();      
            LOG("Slave %d heartbeat OK", actual_slave_id);
        } 
        else {
            LOG_W("[heartbeat]invalid response form %d slave", actual_slave_id);
            LOG_W("head:0x%02X,type:0x%02X,id:0x%02X", reply->frame_head,reply->frame_type,reply->slave_id);
            process_heartbeat_error(slave_id);
        }
    } 
    else {
        LOG_W("Slave %d heartbeat comm error, status=%d", actual_slave_id, status);
        process_heartbeat_error(slave_id);
    }
    s_heartbeat_state     = HEARTBEAT_IDLE;
    s_heartbeat_last_tick = bsp_GetRunTime();
    //LOG("Slave %d heartbeat completed", actual_slave_id);
}

/**
 * @brief get slave heartbeat status
 * @param slave_id  :slave id (1-based)
 * @return slave_status_t 
 */
slave_status_t get_slave_heartbeat_status(uint8_t slave_id)
{
    if (slave_id == 0 || slave_id > SLAVE_SPI_MAX_NUM) {
        return SLAVE_STATUS_UNKNOWN;
    }
    return s_slave_heartbeat[slave_id - 1].status;
}


/**
 * @brief 查询指定从机是否在线
 * @param slave_id 从机ID (1~SLAVE_SPI_MAX_NUM)
 * @return true=在线, false=离线或ID无效
 */
bool is_slave_online_by_id(uint8_t slave_id)
{
    if (slave_id < 1 || slave_id > SLAVE_SPI_MAX_NUM) {
        return false;
    }
    return s_slave_heartbeat[slave_id - 1].status == SLAVE_STATUS_ONLINE;
}


/**
 * @brief 重置心跳统计信息
 */
void reset_heartbeat_info(void)
{
    memset((void*)s_slave_heartbeat, 0, sizeof(s_slave_heartbeat));
    s_heartbeat_state       = HEARTBEAT_IDLE;
    s_heartbeat_last_tick   = bsp_GetRunTime();
    LOG("Heartbeat info reset");
}


void app_master_handle(void)
{
    master_find_slave_process();
	master_heartbeat_process();
	prog_sync_process();
}


void app_spi_master_handle_init(void)
{
    app_master_command_param_init();
}

/****************************************************************************************************/
/* end of hearbeat function																			*/
/****************************************************************************************************/
#if 1
/*static*/ uint8_t s_prog_sync_slave_list[SLAVE_SPI_MAX_NUM]            = {0};
static uint8_t s_prog_sync_slave_total                              = 0;
static uint8_t s_prog_sync_current_index                            = 0;
#if 1
//prog sync parameters
volatile bool                   g_start_prog_sync                   = false;
static prog_sync_state_t        s_prog_sync_state                   = PROG_SYNC_IDLE;
static uint8_t                  s_current_prog_sync_slave_id        = 0;  
static uint32_t                 s_prog_sync_time                    = 0;
static uint8_t                  s_prog_sync_retry_count             = 0;
// sync data
static uint8_t                  g_prog_sync_total_num               = 0;
static uint8_t                  g_prog_sync_cur_step                = 0;
static uint8_t                  g_prog_mode                         = 0;
static uint16_t                 g_prog_sync__interval               = 0;
static uint8_t                  g_prog_slave_sync_mode              = 0;    //0- SYNC 1-STANDLONE
#endif

/**
 * @brief build prog sync slave list
 * @return slave count
 * @note   if find no strobe machine input it's id (1~8) into list
 */
uint8_t build_prog_sync_slave_list(void)
{
    uint8_t count = 0;
    uint8_t slave_num = get_found_slave_count();

    for (uint8_t i = 0; i < slave_num; i++) {
        if (g_machine_type[i] != STROBE_MACHINE) {
            s_prog_sync_slave_list[count] = i + 1;
            count++;
        }
    }
    LOG("Add %d slave to prog sync list", s_prog_sync_slave_total);
    s_prog_sync_slave_total = count;
    return count;
}

/**
 * @brief get need prog sync slave id
 * @param id : 0~7
 * @return slave id (1~8)
 */
uint8_t get_need_prog_sync_slave_id(uint8_t id)
{
    if (id > SLAVE_SPI_MAX_NUM) id = 8;

    return s_prog_sync_slave_list[id];
}

void start_prog_sync(void)
{
    g_start_prog_sync = true;
}

static void prog_sync_process(void)
{
    switch (s_prog_sync_state)
    {
    case PROG_SYNC_IDLE:
        if(g_start_prog_sync){
            s_prog_sync_state               = PROG_SYNC_STARTING;
        }
        break;

    case PROG_SYNC_STARTING:
	{
        if (build_prog_sync_slave_list() == 0) {
            LOG_E("No valid slaves for prog sync, stopping");
            s_prog_sync_state               = PROG_SYNC_STOPPED;
            break;
        }
        g_prog_sync_total_num               = bsp_prog_trig_get_totsl_steps();
        g_prog_sync_cur_step                = bsp_prog_trig_get_current_step();
        g_prog_mode                         = bsp_prog_trig_get_mode();
        g_prog_sync__interval               = bsp_prog_trig_get_interval();
        g_prog_slave_sync_mode              = get_programmable_sync_mode_value();
        LOG_I("Start prog sync: total=%d, cur=%d, mode=%d, interval=%d , slave_sync_mode=%d", 
                g_prog_sync_total_num, g_prog_sync_cur_step, 
                g_prog_mode, g_prog_sync__interval, g_prog_slave_sync_mode);

        s_prog_sync_current_index           = 0; 
        s_prog_sync_retry_count             = 0;
        s_prog_sync_time                    = bsp_GetRunTime();
        // Wait a bit for any pending SPI operations to complete
        if (bsp_spi_is_bus_locked()) {
            if (bsp_CheckRunTime(s_prog_sync_time) < 50) {
                // Wait for bus to be free, max 50ms
                break;
            }
            LOG_W("SPI bus still locked after 50ms, proceeding anyway");
        }

        uint8_t first_slave_id              = s_prog_sync_slave_list[s_prog_sync_current_index];
        if (send_prog_slave_command(first_slave_id) == SPI_MASTER_OK){
            s_current_prog_sync_slave_id    = first_slave_id;
            s_prog_sync_state               = PROG_SYNC_WAITING_REPLY;
        }
        else{
            s_prog_sync_state               = PROG_SYNC_RETRY_DELAY;
            s_prog_sync_retry_count++;
            s_prog_sync_time                = bsp_GetRunTime();
        }
	}
    break;

    case PROG_SYNC_WAITING_REPLY:
        if (bsp_CheckRunTime(s_prog_sync_time) > PROG_SYNC_TIMEOUT_MS){
            s_prog_sync_retry_count++;
            if (s_prog_sync_retry_count >= RETRY_MAX_COUNT){
                s_prog_sync_state           = PROG_SYNC_STOPPED;
                LOG_E("Max retry count reached for slave %d after timeout", s_current_prog_sync_slave_id);
            }
            else{
                s_prog_sync_time            = bsp_GetRunTime();
                s_prog_sync_state           = PROG_SYNC_RETRY_DELAY;
                LOG_W("Timeout retry %d for slave %d", s_prog_sync_retry_count, s_current_prog_sync_slave_id);
            }
        }
        break;

    case PROG_SYNC_RETRY_DELAY:
        if (bsp_CheckRunTime(s_prog_sync_time) >= PROG_SYNC_RETRY_DELAY_MS) {
            uint8_t slave_id                = s_prog_sync_slave_list[s_prog_sync_current_index];
            if (send_prog_slave_command(slave_id) == SPI_MASTER_OK) {
                s_prog_sync_state = PROG_SYNC_WAITING_REPLY;
            }
            else{
                s_prog_sync_retry_count++;
                if (s_prog_sync_retry_count >= RETRY_MAX_COUNT) {
                    s_prog_sync_state       = PROG_SYNC_NEXT_SLAVE;
                    LOG_E("Max retry count reached for slave %d, next slave", slave_id);
                }
                else{
                    s_prog_sync_time        = bsp_GetRunTime();
                    LOG_W("Retry %d for slave %d", s_prog_sync_retry_count, slave_id);
                }
            }
        }
        break;

    case PROG_SYNC_NEXT_SLAVE:
        s_prog_sync_current_index++;
        s_prog_sync_retry_count             = 0;
        if (s_prog_sync_current_index >= s_prog_sync_slave_total){
            s_prog_sync_state               = PROG_SYNC_STOPPED;
            LOG_I("Prog sync finished for %d slaves", s_prog_sync_slave_total);
        }
        else{
            uint8_t next_slave_id           = s_prog_sync_slave_list[s_prog_sync_current_index];
            if (send_prog_slave_command(next_slave_id) == SPI_MASTER_OK){
                s_current_prog_sync_slave_id = next_slave_id;
                s_prog_sync_state           = PROG_SYNC_WAITING_REPLY;
                s_prog_sync_time            = bsp_GetRunTime();
            }
            else{
                s_prog_sync_time            = bsp_GetRunTime();
                s_prog_sync_state           = PROG_SYNC_RETRY_DELAY;
                s_prog_sync_retry_count     = 1;
                LOG_W("Failed to send to slave %d, entering retry", s_current_scan_slave_id);
            }
        }
        break;

    case PROG_SYNC_STOPPED:
        g_start_prog_sync                   = false;
        s_prog_sync_state                   = PROG_SYNC_IDLE;
        s_prog_sync_current_index           = 0;
        s_prog_sync_retry_count             = 0;  // Reset retry count
	    LOG("Prog sync stopped!");
        break;

    default:
        break;
    }
}


static spi_master_status_t send_prog_slave_command(uint8_t slave_id)
{
    if(slave_id > SLAVE_SPI_MAX_NUM)return SPI_MASTER_ERRORPARAMETER;

    if (bsp_spi_is_bus_locked()) {
        return SPI_MASTER_ERROR;
    }
    
    uint8_t slave_index0 = slave_id - 1;
    if (bsp_spi_get_status(slave_index0) != SPI_STATUS_OK) {
        return SPI_MASTER_ERROR; 
    }

    //帧头 类型  版本 从机ID  功能  命令  总步数  当前步数  间隔ms   工作模式  
    // AA   00   01   1~8    03    22    1~64    1~64    2字节      0~1
    uint8_t frame[SPI_FORWARD_FRAME_TOTAL] = {0};
    spi_payload_t *payload          = (spi_payload_t *)&frame[0];
    spi_function_t *func_payload    = (spi_function_t *)payload->reserved;

    payload->frame_head             = SPI_FRAME_HEAD;
    payload->frame_type             = FRAME_TYPE_CMD;
    payload->protocol_version       = SPI_COMM_VERSION;
    payload->slave_id               = slave_id;
    func_payload->function          = FUNC_EXPAND;
    func_payload->command           = CMD_PARAM_PROG_SYNC;

    uint8_t *data                   = payload->reserved;
    data[2]                         = g_prog_sync_total_num;
    data[3]                         = (g_prog_sync_cur_step + 1);
    data[4]                         = g_prog_sync__interval & 0xFF;
    data[5]                         = (g_prog_sync__interval >> 8) & 0xFF; 
    data[6]                         = g_prog_mode;
    data[7]                         = g_prog_slave_sync_mode;

    payload->bcc = bcc_xor((uint8_t*)payload, sizeof(spi_payload_t) - 1);

    spi_status_t result             = bsp_spi_comm( slave_index0, 
                                        (uint8_t *)payload, 
                                        sizeof(spi_payload_t), 
                                        SPI_MASTER_DEFAULT_RESP_LEN,
                                        SPI_MASTER_FIND_SLAVE_MS,
                                        SPI_MASTER_FIND_SLAVE_TIMEOUT,
                                        master_cmd_prog_sync_slave_callback);                   

    if (result != SPI_STATUS_OK){
        LOG_E("Error:can not send prog sync command");
        return SPI_MASTER_ERROR;
    }
    LOG("Sent prog sync command to slave %d", slave_id);
    s_prog_sync_time                = bsp_GetRunTime();
    return SPI_MASTER_OK;
}

static void master_cmd_prog_sync_slave_callback(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t receive_len)
{
    // 确保回调对应的是当前正在等待的从机
    if (s_prog_sync_state != PROG_SYNC_WAITING_REPLY || slave_id != (s_current_prog_sync_slave_id - 1)) {
        LOG_E ("[Prog SYNC]Unexpected callback for slave %d in state %d", slave_id + 1, s_prog_sync_state);
        return;
    }

    uint8_t *data                   = response_data;
    if (status == SPI_STATUS_OK && response_data != NULL) {
        spi_payload_t *reply        = (spi_payload_t *)response_data;
        
        if (reply->frame_head == SPI_FRAME_HEAD && 
            reply->frame_type == FRAME_TYPE_ACK &&
            reply->slave_id   == s_current_prog_sync_slave_id) {

            uint8_t * data          = reply ->reserved;            
            if (data[2] == g_prog_sync_total_num                &&
                data[3] == (g_prog_sync_cur_step + 1)           &&
                data[4] == g_prog_sync__interval & 0xFF         &&
                data[5] == (g_prog_sync__interval >> 8) & 0xFF  &&
                data[6] == g_prog_mode                          &&
                data[7] == g_prog_slave_sync_mode) {
                s_prog_sync_state        = SCAN_NEXT_SLAVE;
            }
        } 
        else {
            s_prog_sync_retry_count++;
            if(s_prog_sync_retry_count >= RETRY_MAX_COUNT) {
                LOG_E("Max retry count reached for slave %d after invalid response", s_current_prog_sync_slave_id);
                s_prog_sync_state   = PROG_SYNC_STOPPED;
            } 
            else {
                s_prog_sync_time    = bsp_GetRunTime();
                s_prog_sync_state   = PROG_SYNC_RETRY_DELAY;
                LOG_W("Invalid response retry %d for slave %d", s_prog_sync_retry_count, s_current_prog_sync_slave_id);
                LOG_E("Prog Sync received invalid frame: 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",
                data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
        
            }
        }
    } 
    else {
        s_prog_sync_retry_count++;
        if(s_prog_sync_retry_count >= RETRY_MAX_COUNT) {
            LOG_E("Max retry count reached for slave %d after comm error", s_current_prog_sync_slave_id);
            s_prog_sync_state = PROG_SYNC_STOPPED;
        } 
        else {
            s_prog_sync_time       = bsp_GetRunTime();
            s_prog_sync_state       = PROG_SYNC_RETRY_DELAY;
            LOG_W("Comm error retry %d for slave %d", s_prog_sync_retry_count, s_current_prog_sync_slave_id);
        }
    }
}

#endif




//******************************** Declaring ********************************//


