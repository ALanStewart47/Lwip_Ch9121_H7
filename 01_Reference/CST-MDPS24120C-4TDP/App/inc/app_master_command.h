/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file    app_master_command.h
 *
 * @author  Alan | R&D Dept. | CST
 *
 * @brief   master send command to slave, control or acquire slave.
 *
 * Processing flow:
 * call directly.
 *
 * @version     V1.0    2025-10-22   ALan      add find slave function
 *              V1.1    2025-10-28   ALan      add heartbeat function
 * @note 
 *
 *****************************************************************************/
#ifndef __APP_MASTER_COMMAND_H_
#define __APP_MASTER_COMMAND_H_

//******************************** Includes *********************************//
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
//******************************** Includes *********************************//

//******************************** Defines **********************************//
//find slave
#define RETRY_MAX_COUNT                 3
#define RETRY_DELAY_MS                  5   // 重试间隔时间，例如5ms
#define PROG_SYNC_RETRY_DELAY_MS        2  
#define FIND_SLAVE_TIMEOUT_MS           20   // 查找从机的超时时间，例如20ms
#define PROG_SYNC_TIMEOUT_MS            20   // 查找从机的超时时间，例如20ms

//heartbeat 
#define HEARTBEAT_INTERVAL_MS           10000    
#define HEARTBEAT_ERROR_THRESHOLD       3       

//find slave
typedef enum {
    MASTER_CMD_STATE_IDLE,          // 空闲
    MASTER_CMD_STATE_WAITING_REPLY, // 等待回复
    MASTER_CMD_STATE_SUCCESS,       // 成功
    MASTER_CMD_STATE_TIMEOUT,       // 超时
    MASTER_CMD_STATE_ERROR,         // 其他错误
} master_cmd_state_t;

typedef enum {
    SCAN_IDLE,
    SCAN_STARTING,
    SCAN_WAITING_REPLY,
    SCAN_RETRY_DELAY,
    SCAN_NEXT_SLAVE,
    SCAN_STOPPED,
} master_scan_state_t;

typedef enum {
  SCAN_RESULT_NONE = 0,
  SCAN_RESULT_SUCCESS,
  SCAN_RESULT_STOPPED,
}scan_result_t;


///////heartbeat 
typedef enum {
    SLAVE_STATUS_UNKNOWN = 0,   // 未知状态
    SLAVE_STATUS_ONLINE,        // 在线正常
    SLAVE_STATUS_ERROR,         // 故障
} slave_status_t;

typedef enum {
    DIGIT_MACHINE           = 0x00,
    STROBE_MACHINE          = 0x01,
    POINT_5V_MACHINE        = 0x02,
}machine_type_t;

typedef struct {
    slave_status_t  status;             // 从机状态
    uint8_t         error_count;        // 连续错误计数
    uint32_t        last_success_tick;  // 最后成功时间
} slave_heartbeat_info_t;

typedef enum {
    HEARTBEAT_IDLE = 0,
    HEARTBEAT_SENDING,      // 正在发送心跳
    HEARTBEAT_WAITING,      // 等待回复
    HEARTBEAT_DELAY,        // 下一轮延时
} heartbeat_state_t;
///////heartbeat 

typedef enum {
    PROG_SYNC_IDLE,
    PROG_SYNC_STARTING,
    PROG_SYNC_WAITING_REPLY,
    PROG_SYNC_RETRY_DELAY,
    PROG_SYNC_NEXT_SLAVE,
    PROG_SYNC_STOPPED,
} prog_sync_state_t;
///////prog_sync

typedef enum
{
  SPI_MASTER_OK                = 0,           /* Operation completed successfully.  */
  SPI_MASTER_ERROR             = 1,           /* Run-time error without case matched*/
  SPI_MASTER_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  SPI_MASTER_ERRORRESOURCE     = 3,           /* Resource not available.            */
  SPI_MASTER_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  SPI_MASTER_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  SPI_MASTER_ERRORISR          = 6,           /* Not allowed in ISR context         */
  SPI_MASTER_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}spi_master_status_t;


//******************************** Defines **********************************//

//******************************** Declaring ********************************//
uint8_t get_found_slave_count(void);
scan_result_t get_scan_result(void);

uint8_t get_slave_count(void);

void clear_scan_result(void);
scan_result_t get_and_clear_scan_result(void);
spi_master_status_t master_find_slave_ctrl(bool on_off);
master_scan_state_t get_scan_state(void);
void app_master_handle(void);
void app_spi_master_handle_init(void);
slave_status_t get_slave_heartbeat_status(uint8_t slave_id);
bool is_slave_online_by_id(uint8_t slave_id);
void start_prog_sync(void);
uint8_t build_prog_sync_slave_list(void);
uint8_t get_need_prog_sync_slave_id(uint8_t id);

void app_master_heartbeat_resume(void);
void app_master_heartbeat_pause(void);
//******************************** Declaring ********************************//

#endif /* __APP_MASTER_COMMAND_H_ */


