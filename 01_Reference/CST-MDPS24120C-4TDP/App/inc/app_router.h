/*
 * Channel router: decide master vs. slave, repack and forward via SPI,
 * and send local frames into existing protocol stack.
 *
 * No changes are made in Protocol/.
 */
#ifndef __APP_ROUTER_H_
#define __APP_ROUTER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Build fixed-length 23-byte SPI outer frame and send
#define SPI_FORWARD_FRAME_TOTAL         26
#define SPI_FORWARD_INNER_MAX           (SPI_FORWARD_FRAME_TOTAL - 5)
#define SPI_FRAME_HEAD                  0xAA
#define SPI_COMM_VERSION                0x01

// 最大重试次数
#define SPI_BRIDGE_MAX_RETRY_COUNT      3
// 默认超时时间（100us 时基）最大可设到100，超过100需修改
#define SPI_MASTER_DEFAULT_TIMEOUT_MS   30
// 主机发起二次通讯（回复）的默认延时时间（100us 时基）
#define SPI_MASTER_DEFAULT_DELAY_MS     2
//同上
#define SPI_MASTER_FIND_SLAVE_MS        2	
#define SPI_MASTER_FIND_SLAVE_TIMEOUT   20	
#define SPI_MASTER_COMM_DELAY_MS        2	
#define SPI_MASTER_COMM_TIMEOUT_MS      20	
// 默认应答长度（字节）
#define SPI_MASTER_DEFAULT_RESP_LEN     26
// 应用层额外超时缓冲（毫秒）
#define SPI_MASTER_APP_TIMEOUT_BUFFER   1000

#define HEAD_SINGLE                     0xCA
#define HEAD_MULTI                      0xCB
#define HEAD_PROG                       0xCC
#define RETRY_QUEUE_SIZE                8      // queue size enough to hold 100us*8 = 0.8ms of commands
#define MAX_RETRY_COUNT                 3  
#define MAX_BCC_RETRY_COUNT             5 
#define SYNC_WAIT_TIMEOUT_MS            100     


// frame type for SPI communication
typedef enum {
    FRAME_TYPE_CMD                      = 0x00, // 命令
    FRAME_TYPE_ACK                      = 0x01, // ACK
    FRAME_TYPE_BCC_ERR                  = 0x02, // 校验错
    FRAME_TYPE_HEAD_ERR                 = 0x03, // 帧头错误
    FRAME_TYPE_LEN_ERR                  = 0x04, // 长度错误
    FRAME_TYPE_FUNC_ERR                 = 0x05, // 非法功能
    FRAME_TYPE_CMD_ERR                  = 0x06, // 非法命令
    FRAME_TYPE_SLAVE_ERR                = 0x07, // 非法从机
} spi_frame_type_t;

// function code
typedef enum {
    FUNC_SYSTEM_INFO                    = 0x01,
    FUNC_SYSTEM_CTRL                    = 0x02,
    FUNC_EXPAND                         = 0x03,
}spi_function_type_t;

typedef enum {
    CMD_SYS_HEARTBEAT                   = 0x01, 
    CMD_SYS_FIND_SLAVE                  = 0x02, 
    CMD_SYS_GET_BASIC                   = 0x03, 
    CMD_SYS_MASTER_INFO                 = 0x04,
    CMD_SYS_GET_VERSION                 = 0x05, 
} spi_sys_info_cmd_t;

typedef enum {
    CMD_SYS_SWITCH_BOOT                 = 0x01, 
    CMD_SYS_REBOOT                      = 0x02, 
    CMD_SYS_GET_REBOOT                  = 0x03, 
    CMD_SYS_LOCK_SLAVE                  = 0x04, 
    CMD_SYS_UNLOCK_SLAVE                = 0x05, 
} spi_sys_ctrl_cmd_t;

typedef enum {
    CMD_GET_RECEIVE_NUM                 = 0x01, 
    CMD_CLEAR_RECEIVE_NUM               = 0x02, 
    CMD_UPGRADE_PACK_NUM                = 0x03, 
    CMD_CHANGE_COM_LENGTH               = 0x04, 
    CMD_COM_INTERVAL                    = 0x05, 
}spi_expand_cmd_t;

typedef enum {  
    RESP_NONE                           = 0, 
    RESP_UART                              , 
    RESP_TCP 
} resp_target_t;

#pragma pack(1)
typedef struct {
    uint8_t             frame_head;
    uint8_t             frame_type;
    uint8_t             protocol_version;
    uint8_t             slave_id;
    uint8_t             reserved[21]; // 填充到21字节
    uint8_t             bcc;
} spi_payload_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t             function;
    uint8_t             command;
    uint16_t            data1;
    uint16_t            data2;
} spi_function_t;
#pragma pack()

// Retry queue, 
typedef struct {
    uint8_t             slave_id;                           // (0-7)
    uint8_t             frame[SPI_FORWARD_FRAME_TOTAL];     // 
    resp_target_t       target;                             // (UART/TCP)
    uint8_t             target_id;                          // (uart num/socket num)
    uint8_t             silent;                             // silent flag
    uint8_t             retry_count;                        // retry count 
} retry_queue_item_t;

typedef struct {
    retry_queue_item_t  items[RETRY_QUEUE_SIZE];
    uint8_t             head;          
    uint8_t             tail;          
    uint8_t             count;         
} retry_queue_t;


// Router initialization and periodic processing
void router_init        (void);       // Initialize router module (call once at startup)
void router_handle      (void);       // Process retry queue (call in main loop)
void router_process_uart(uint8_t uart_number, const uint8_t *buf, uint16_t len);
void router_process_tcp (uint8_t socket_sn, const uint8_t *buf, uint16_t len);
// SPI reply hook called by SPI bridge callback
void router_on_spi_reply(uint8_t slave_id, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif

