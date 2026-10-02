
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
 * @author ALan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the flash read and write.
 *
 * Processing flow:
 * call directly.
 *
 * @version     V1.0    2025-09-30  ALan
 *              V1.1    2025-10-29  ALan    
 *              V1.2    2025-11-10  ALan     fixed silent mode bug , add public commands forward to slaves, more flexible data responses
 *              V1.3    2025-12-05  ALan     add sync wait ctrl   
 *              V1.4    2025-12-18  ALan     CA command routing optimization
 * @note tcp or uart data routing to slaves
 *       Channel router implementation. Parses CA/CB frames, splits by channel,
 *       forwards to slaves via SPI with fixed 26-byte frames, and processes local
 *       frames via existing protocol functions (input_data/analysis_command).
 *
 *****************************************************************************/

 //******************************** Includes *********************************//
 #include "app_router.h"
 #include <string.h>
 #include <stdbool.h>
 #include "protocol_public.h"       // CHANNEL_NUM, package & protocol APIs
 #include "bsp_spi_bus_dma.h"       // SLAVE_SPI_MAX_NUM
 #include "app_uart.h"              // DMA_Usart_Send
#if W5500_IS_ON
 #include "socket.h"                // send(sn,...)
#endif
#include "rtt_log.h"
#include "app_slave_param_sync.h"  
#include "spi.h"
 //******************************** Includes *********************************//

 //******************************** Defines **********************************//
extern uint8_t                  g_spi_error_pending;

static volatile resp_target_t   s_spi_resp_target[SLAVE_SPI_MAX_NUM]    = {RESP_NONE};
static volatile uint8_t         s_spi_resp_uart  [SLAVE_SPI_MAX_NUM]    = {0};
static volatile uint8_t         s_spi_resp_sock  [SLAVE_SPI_MAX_NUM]    = {0};
static volatile uint8_t         s_spi_resp_silent[SLAVE_SPI_MAX_NUM]    = {0};       // silent flag, 1:don't forward slave response
// BCC check retry   
static uint8_t                  s_last_sent_frame[SLAVE_SPI_MAX_NUM][SPI_FORWARD_FRAME_TOTAL];   // save last sent frame buf
static uint8_t                  s_bcc_retry_count[SLAVE_SPI_MAX_NUM]    = {0};                      // retry count
static retry_queue_t            s_retry_queue                           = {0};

///sync waiting ctrl param 
static bool                     s_sync_wait_active                      = false;    // 是否处于等待同步完成状态
static uint8_t                  s_sync_wait_count                       = 0;        // 剩余需要等待的从机数量
static uint32_t                 s_sync_wait_start_tick                  = 0;        // 等待开始时间(用于超时保护)
static uint8_t                  s_pending_reply_buf[SUM_SIZE]           = {0};      // 暂存回复数据的缓冲区
static uint16_t                 s_pending_reply_len                     = 0;        // 暂存数据长度
static uint8_t                  s_pending_reply_type                    = 0;        // 0:None, 1:UART, 2:TCP
static uint8_t                  s_pending_reply_id                      = 0;        // UART端口号 或 Socket ID
///sync waiting ctrl param 

static void router_spi_cb               (uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t response_len);
static void spi_forward_send_with_origin(uint8_t slave_index0, const uint8_t *inner, uint16_t inner_len,
                                         resp_target_t target, uint8_t target_id, bool silent);
static void retry_queue_init            (void);
static bool retry_queue_is_empty        (void);
static bool retry_queue_is_full         (void);
static bool retry_queue_push            (uint8_t slave_id, const uint8_t *frame, 
                                            resp_target_t target, uint8_t target_id, bool silent);
static void retry_queue_process         (void);

//******************************** Defines **********************************//

//******************************** Declaring ********************************//

static inline uint8_t bcc_xor(const uint8_t *buf, int len)
{
    uint8_t c = 0;
    for (int i = 0; i < len; i++) c ^= buf[i];
    return c;
}

/**
 * @brief Map global channel to slave ID and local channel
 * @param global_ch     Global channel (1~N)
 * @param out_slave_id  Output slave ID (0~8, 0=master)
 * @param out_local_ch  Output local channel (1~CHANNEL_NUM)
 */
static inline void map_channel(uint8_t global_ch, uint8_t *out_slave_id, uint8_t *out_local_ch)
{
    // 0 => master, 1 => slave index
    uint8_t idx   = (global_ch - 1) / CHANNEL_NUM; 
    uint8_t local = (uint8_t)(((global_ch - 1) % CHANNEL_NUM) + 1);
    *out_slave_id = idx;                 // 0..8 (0 is master/local)
    *out_local_ch = local;               // 1..CHANNEL_NUM
}

///V1.3
/**
 * @brief Flush pending reply data
 * @note  Called when sync wait is over
 */
static void router_flush_pending_reply(void)
{
    if (s_pending_reply_len > 0) {
        if (s_pending_reply_type == 1) {        // UART
             DMA_Usart_Send(s_pending_reply_id, s_pending_reply_buf, s_pending_reply_len);
        }
        #if W5500_IS_ON
            else if (s_pending_reply_type == 2) {   // TCP
                send(s_pending_reply_id, s_pending_reply_buf, s_pending_reply_len);
            }
        #endif
        s_pending_reply_len = 0;
    }
    s_sync_wait_active      = false;
    s_pending_reply_type    = 0;
    app_master_heartbeat_resume(); 
}

/**
 * @brief Check sync wait status
 * @note  Called periodically in main loop or communication callbacks
 */
static void router_check_sync_wait(void)
{
    if (!s_sync_wait_active) return;

    if (s_sync_wait_count == 0 || 
        (bsp_GetRunTime() - s_sync_wait_start_tick > SYNC_WAIT_TIMEOUT_MS)) 
    {
        router_flush_pending_reply();
    }
}

/////////V1.1/////////
/**
 * @brief Get available slave count for router
 * @return available slave count
 */
static inline uint8_t router_available_slave_count(void)
{
    uint8_t count = get_found_slave_count();
    if (count > SLAVE_SPI_MAX_NUM) {
        count = SLAVE_SPI_MAX_NUM;
    }
    return count;
}

/**
 * @brief Check if slave ID is valid for router
 * @param sid Slave ID (1~8)
 * @param available Available slave count
 * @return true=valid, false=invalid
 */
static inline bool router_is_valid_slave(uint8_t sid, uint8_t available)
{
    return (sid >= 1U) && (sid <= available);
}

/**
 * @brief retry queue initialization
 */
static void retry_queue_init(void)
{
    memset(&s_retry_queue, 0, sizeof(s_retry_queue));
}

/**
 * @brief check if the queue is empty
 */
static bool retry_queue_is_empty(void)
{
    return s_retry_queue.count == 0;
}

/**
 * @brief check if the queue is full 
 */
static bool retry_queue_is_full(void)
{
    return s_retry_queue.count >= RETRY_QUEUE_SIZE;
}

/**
 * @brief push SPI command into retry queue
 * @param slave_id (0-7)
 * @param frame  (26字节)
 * @param target  response target
 * @param target_id target ID
 * @param silent silent flag
 * @return true=success , false=queue full 
 */
static bool retry_queue_push(uint8_t slave_id, const uint8_t *frame, 
                              resp_target_t target, uint8_t target_id, bool silent)
{
    if (retry_queue_is_full()) {
        LOG_E("Retry queue full, drop command for slave %d", slave_id + 1);
        return false;
    }

    retry_queue_item_t *item    = &s_retry_queue.items[s_retry_queue.tail];
    item->slave_id              = slave_id;
    memcpy(item->frame, frame, SPI_FORWARD_FRAME_TOTAL);
    item->target                = target;
    item->target_id             = target_id;
    item->silent                = silent ? 1 : 0;
    item->retry_count           = 0;

    s_retry_queue.tail          = (s_retry_queue.tail + 1) % RETRY_QUEUE_SIZE;
    s_retry_queue.count++;

    LOG("Retry queue push: slave %d, count=%d", slave_id + 1, s_retry_queue.count);
    return true;
}

/**
 * @brief  pop retry SPI command
 * @param  out_item retry_queue_item_t
 * @return true=success , false=queue empty
 */
/*static bool retry_queue_pop(retry_queue_item_t *out_item)
{
    if (retry_queue_is_empty()) {
        return false;
    }

    *out_item = s_retry_queue.items[s_retry_queue.head];
    s_retry_queue.head  = (s_retry_queue.head + 1) % RETRY_QUEUE_SIZE;
    s_retry_queue.count--;
    return true;
}*/

/**
 * @brief retry queue processing function (called in main loop)
 * @note 
 */
static void retry_queue_process(void)
{
    if (retry_queue_is_empty()) {
        return;  
    }
    retry_queue_item_t *item = &s_retry_queue.items[s_retry_queue.head];
	
	__NOP();__NOP();__NOP();__NOP();

    spi_status_t ret = bsp_spi_comm(item->slave_id, item->frame, SPI_FORWARD_FRAME_TOTAL,
                                     SPI_MASTER_DEFAULT_RESP_LEN, 
                                     SPI_MASTER_DEFAULT_DELAY_MS,
                                     SPI_MASTER_DEFAULT_TIMEOUT_MS,
                                     router_spi_cb);

    if (ret == SPI_STATUS_OK) {
        s_spi_resp_target[item->slave_id] = item->target;
        s_spi_resp_silent[item->slave_id] = item->silent;
        if (item->target == RESP_UART) s_spi_resp_uart[item->slave_id] = item->target_id;
        if (item->target == RESP_TCP)  s_spi_resp_sock[item->slave_id] = item->target_id;

        s_retry_queue.head  = (s_retry_queue.head + 1) % RETRY_QUEUE_SIZE;
        s_retry_queue.count--;

        LOG("Retry queue send success: slave %d, remain=%d", item->slave_id + 1, s_retry_queue.count);
    } 
    else if (ret == SPI_STATUS_BUSY) {
        // SPI总线繁忙: 保持在队列中,下次继续尝试
        // 不做任何操作,等待下次调用
    } 
    else {
        item->retry_count++;
        if (item->retry_count >= MAX_RETRY_COUNT) {
            LOG_E("Retry queue max retry reached for slave %d, drop command", item->slave_id + 1);

            s_retry_queue.head = (s_retry_queue.head + 1) % RETRY_QUEUE_SIZE;
            s_retry_queue.count--;
        } else {
            LOG_W("Retry queue send failed: slave %d, retry %d/%d", 
                  item->slave_id + 1, item->retry_count, MAX_RETRY_COUNT);
        }
    }
}

// ====================== Local/master side handling helpers ======================

// Accumulator to feed original protocol in-order
static uint8_t  s_local_acc[SUM_SIZE];
static uint16_t s_local_len = 0;

static inline void local_acc_reset(void) 
{ 
    s_local_len = 0; 
}

static inline void local_acc_append(const uint8_t *p, uint16_t n)
{
    if (n == 0) return;
    if (s_local_len + n > sizeof(s_local_acc)) 
        n = (uint16_t)(sizeof(s_local_acc) - s_local_len);

    memcpy(&s_local_acc[s_local_len], p, n);
    s_local_len = (uint16_t)(s_local_len + n);
}

unsigned char *p_uart_tx ;
/**
 * @brief Process local data and reply via UART.
 * @note
 */
static inline void process_local_and_reply_uart(uint8_t uart_number)
{
    if (s_local_len == 0) return;
    input_data(s_local_acc, s_local_len);
    unsigned int tx_len = 0;
    analysis_command();
    //unsigned char *tx = get_prepare_tx_buffer(&tx_len);
	p_uart_tx  = get_prepare_tx_buffer(&tx_len);
    if (tx_len > 0) {
        if (s_sync_wait_active) {
            
           // if (tx_len > sizeof(s_pending_reply_buf)) tx_len = sizeof(s_pending_reply_buf);
            memcpy(s_pending_reply_buf, p_uart_tx, tx_len);
            s_pending_reply_len     = (uint16_t)tx_len;
            s_pending_reply_type    = 1;                // UART
            s_pending_reply_id      = uart_number;
            LOG_I("[SYNC]Pending TCP reply for socket %d, len=%d", sn, tx_len);
        } 
        else {
            DMA_Usart_Send(uart_number, p_uart_tx, (uint16_t)tx_len);
        }
    }
    local_acc_reset();
}

/**
 * @brief Process local data and reply via TCP.
 * @note
 */
#if W5500_IS_ON
static inline void process_local_and_reply_tcp(uint8_t sn)
{
    if (s_local_len == 0) return;
    input_data(s_local_acc, s_local_len);
    unsigned int tx_len = 0;
    analysis_command();
    unsigned char *tx = get_prepare_tx_buffer(&tx_len);
    
    if (tx_len > 0) {
        if (s_sync_wait_active) {
            // 挂起：存入缓冲区
            //if (tx_len > sizeof(s_pending_reply_buf)) tx_len = sizeof(s_pending_reply_buf);
            memcpy(s_pending_reply_buf, tx, tx_len);
            s_pending_reply_len     = (uint16_t)tx_len;
            s_pending_reply_type    = 2;       // TCP
            s_pending_reply_id      = sn;
            LOG_I("[SYNC]Pending TCP reply for socket %d, len=%d", sn, tx_len);
        } 
        else {
            send(sn, (uint8_t *)tx, (uint16_t)tx_len);
        }
    }
    local_acc_reset();
}
#endif

// ====================== Frame routing ======================

/**
 * @brief Route a single-channel frame (0xCA).
 * @param frm Pointer to the frame data.
 * @param frm_len Length of the frame data.
 * @param is_uart True if the frame originated from UART, false if from TCP.
 * @param origin_id The UART number or TCP socket number where the frame originated.
 * @return None.
 */
static void route_single_frame(const uint8_t *frm, uint16_t frm_len, bool is_uart, uint8_t origin_id)
{
    if (frm_len < 6) { 
        local_acc_append(frm, frm_len); 
        return; 
    }
    if (frm[5] != bcc_xor(frm, 5)) { 
        local_acc_append(frm, 6); 
        return; 
    }

    uint8_t channel = frm[2];
    uint8_t cmd     = frm[1];
    uint8_t sid     = 0, lch = 0;
    bool need_broadcast = false;
    uint8_t need_sync_slave_num = 0;
     
    if ((cmd == 0x03 || cmd == 0x41 || cmd ==0x45) && (channel == 0) ) {
        need_broadcast = true;
    }
    else if ( (cmd == 0x43 || cmd == 0x46 || cmd == 0x47 ) && (channel == 0) ) { 
        need_broadcast = true;
    }   // Set_Channel_Switch, Set_brightness_level
    else if ((cmd == 0x02 || cmd == 0x05) && (channel == 0)) {
        need_broadcast = true;
    }// public commands forward to slaves

    // need to broadcast to slaves
    if (need_broadcast) {
        local_acc_append(frm, 6);
        uint8_t available_slaves = router_available_slave_count();
        uint8_t inner[6] = {0};

        if (available_slaves > 0) {
            inner[0] = frm[0];      // head 0xCA
            inner[1] = frm[1];      // cmd
            inner[2] = frm[2];      // keep orign channel 
            inner[3] = frm[3];
            inner[4] = frm[4];
            inner[5] = bcc_xor(inner, 5);
        }

        if (cmd == 0x03 || cmd == 0x41 || cmd ==0x45) {
            need_sync_slave_num = build_prog_sync_slave_list();
            if (need_sync_slave_num > 0) {
                    for (uint8_t i = 0; i < need_sync_slave_num; i++) {
                    uint8_t slave_id = get_need_prog_sync_slave_id(i) - 1;
                    spi_forward_send_with_origin (slave_id, inner, 6, is_uart ? RESP_UART : RESP_TCP, origin_id, true);
					LOG_I("0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",inner[0],inner[1],inner[2],inner[3],inner[4], inner[5]);
                }
            }
        }
        else {
            // 向所有在线从机广播 (广播指令不需要从机回复)
            for (uint8_t i = 0; i < available_slaves; i++) {
                spi_forward_send_with_origin (i, inner, 6, is_uart ? RESP_UART : RESP_TCP, origin_id, true);
            }
        }
        return;
    }
    
    map_channel(channel, &sid, &lch);
    //sid is master 
    if (sid == 0) {
        local_acc_append(frm, 6);
        return;
    } 
    //sid is slave
    uint8_t available_slaves = router_available_slave_count();
    if (!router_is_valid_slave(sid, available_slaves)) {
        local_acc_append(frm, 6);
        return;
    }
         
    uint8_t inner[6];
    inner[0] = frm[0];      // head 0xCA
    inner[1] = frm[1];      // cmd
    inner[2] = lch;         // slave local channel
    inner[3] = frm[3];
    inner[4] = frm[4];
    inner[5] = bcc_xor(inner, 5);
    spi_forward_send_with_origin ((uint8_t)(sid - 1), inner, 6, is_uart ? RESP_UART : RESP_TCP, origin_id, false);


    if (Set_Brightness == cmd) {
        uint16_t brightness = (inner[3] <<8 ) | inner[4];
        set_slave_cache_brightness(sid,  lch-1, brightness);
    }
    if (Set_Pulse_width == cmd) {
        uint16_t width = (inner[3] <<8 ) | inner[4];
        set_slave_cache_strobe_width(sid, lch-1, width);
    } 
    if (Set_mode == cmd) {
        uint8_t mode = inner[4];
        set_slave_cache_work_mode(sid, mode);
    }
    if (Set_Digital_trigger_mode == cmd) {
        uint8_t state = inner[4];
        set_slave_cache_normal_mode_state(sid, state);
    }

}


/**
 * @brief Route a multi-channel frame (0xCB).
 * @param frm Pointer to the frame data.    
 * @param frm_len Length of the frame data.
 * @param is_uart True if the frame originated from UART, false if from TCP.
 * @param origin_id The UART number or TCP socket number where the frame originated.
 * @return None.
 */
static void route_multi_frame(const uint8_t *frm, uint16_t frm_len, bool is_uart, uint8_t origin_id)
{
    if (frm_len < 4) { 
        local_acc_append(frm, frm_len); 
        return; 
    }
    uint8_t n = frm[2];
    uint16_t expect = (uint16_t)(4 + 3*n);

    if (frm_len < expect) { 
        local_acc_append(frm, frm_len); 
        return; 
    }
    // check bcc number 
    if (frm[expect - 1] != bcc_xor(frm, expect - 1)) { 
        local_acc_append(frm, expect); 
        return; 
    }

    typedef struct { 
        uint8_t ch; 
        uint16_t val; 
    } item_t;

    item_t  local[16]; 
    uint8_t local_cnt = 0;
    item_t  remote[SLAVE_SPI_MAX_NUM][8]; 
    uint8_t remote_cnt[SLAVE_SPI_MAX_NUM] = {0};

    uint8_t available_slaves = router_available_slave_count();
    for (uint8_t i = 0; i < n; i++) 
    {
        uint8_t ch   = frm[3 + 3*i];
        uint16_t val = (uint16_t)((frm[4 + 3*i] << 8) | frm[5 + 3*i]);
        uint8_t sid  = 0, lch = 0; 

        map_channel(ch, &sid, &lch);
        
        if (sid == 0) { //master 
            if (local_cnt < (sizeof local / sizeof local[0]))
                local[local_cnt++] = (item_t){ ch, val };
        } 
        /*
        else if (sid <= SLAVE_SPI_MAX_NUM) 
        {
            uint8_t gi = (uint8_t)(sid - 1);   //sid:1~8
            if (remote_cnt[gi] < (sizeof remote[0] / sizeof remote[0][0]))
                remote[gi][remote_cnt[gi]++] = (item_t){ lch, val };
        }*/
       else{
            if (!router_is_valid_slave(sid, available_slaves)) {
                if (local_cnt < (sizeof local / sizeof local[0])) {
                    local[local_cnt++] = (item_t){ ch, val };
                }
                continue;
            }

            uint8_t gi = (uint8_t)(sid - 1);
            if (remote_cnt[gi] < (sizeof remote[0] / sizeof remote[0][0])) {
                remote[gi][remote_cnt[gi]++] = (item_t){ lch, val };
            }
        }
    }
    // is local part , group package
    if (local_cnt > 0) 
    {
        uint8_t tmp[4 + 3*16];
        uint16_t k = 0;
        tmp[k++]   = frm[0];  // head 0xCB
        tmp[k++]   = frm[1];  // cmd
        tmp[k++]   = local_cnt;
        for (uint8_t i = 0; i < local_cnt; i++) 
        {
            tmp[k++] = local[i].ch;                 
            tmp[k++] = (uint8_t)(local[i].val >> 8);
            tmp[k++] = (uint8_t)(local[i].val & 0xFF);
        }
        tmp[k++] = bcc_xor(tmp, (int)(k));
        local_acc_append(tmp, k);
    }

    //is slave part: Send by slave group,
    // 如果有主机通道，从机回复需要静默(避免多次回复)
    bool has_local = (local_cnt > 0);
    
    for (uint8_t s = 0; s < SLAVE_SPI_MAX_NUM; s++) 
    {
        uint8_t cnt = remote_cnt[s];
        if (cnt == 0) continue;
        uint8_t pos = 0;

        while (pos < cnt) 
        {
            uint8_t max_items = (uint8_t)((SPI_FORWARD_INNER_MAX - 3) / 3); 
            uint8_t chunk     = (uint8_t)(((cnt - pos) > max_items) ? max_items : (cnt - pos));   
            uint8_t inner[SPI_FORWARD_INNER_MAX] = {0};
            uint8_t k = 0;
            inner[k++] = frm[0]; // 0xCB
            inner[k++] = frm[1];
            inner[k++] = chunk;
            for (uint8_t i = 0; i < chunk; i++) 
            {
                inner[k++] = remote[s][pos + i].ch;     // 1..CHANNEL_NUM
                inner[k++] = (uint8_t)(remote[s][pos + i].val >> 8);
                inner[k++] = (uint8_t)(remote[s][pos + i].val & 0xFF);
            }
            inner[k++] = bcc_xor(inner, (int)(3 + 3*chunk));
            
            // 如果有主机通道，从机静默执行(不回复)
            spi_forward_send_with_origin(s, inner, k, is_uart ? RESP_UART : RESP_TCP, origin_id, has_local);
            pos = (uint8_t)(pos + chunk);
        }
    }
}


/**
 * @brief Route a programmable frame (0xCC).
 * @param frm Pointer to the frame data.
 * @param rem Remaining length of the frame data.
 * @param is_uart True if the frame originated from UART, false if from TCP.
 * @param origin_id The UART number or TCP socket number where the frame originated.
 * @return The number of bytes consumed from the frame. Returns 0 if the frame is incomplete.
 * @note This function verifies the frame's integrity and routes it to the appropriate destination:
 */
static uint16_t route_programmable_frame(const uint8_t *frm, uint16_t rem, bool is_uart, uint8_t origin_id)
{
    if (rem < 4) {
        return 0; // incomplete
    }
    uint8_t  cmd         = frm[1];
    uint8_t  recipe      = frm[2];
    uint16_t expect_len  = 0;

    if (cmd == Read_Programmable_data) {
        // Read_Programmable_data: 8 bytes content + 1 CRC
        expect_len = 9;
    } 
    else if (cmd == Set_Programmable_data) 
    {
        // Set_Programmable_data: 12 + channel_count*2 content, +1 CRC
        if (rem < 8) {
            return 0; // incomplete
        }
        uint16_t ch_cnt = (uint16_t)((frm[6] << 8) | frm[7]);
        expect_len = (uint16_t)(13 + ch_cnt * 2);
    } 
    else {
        // Other programmable simple commands: 6 content + 1 CRC
        expect_len = 7;
    }

    if (rem < expect_len) {
        return 0; // incomplete
    }

    if (recipe == 1) {
        // master: pass-through to protocol
        if (get_programmable_sync_mode() == SYNC_MODE_MASTER_SYNC){
            if ( cmd == Set_Prog_Total_steps        || 
                 cmd == Set_Prog_Current_steps      ||
                 cmd == Set_Prog_Trigger_mode       ||
                 cmd == Set_Prog_trigger_interval   ||
                 cmd == Set_Prog_Reset_steps) {

                uint8_t need_sync_slave_num = build_prog_sync_slave_list();

                if (need_sync_slave_num == 0) {
                    // no slaves found, process locally
                    local_acc_append(frm, expect_len);
                    return expect_len;
                }

                if (need_sync_slave_num > 0) {
                    s_sync_wait_active      = true;
                    s_sync_wait_count       = need_sync_slave_num;
                    s_sync_wait_start_tick  = bsp_GetRunTime();
                    // pause heartbeat during sync wait
                    app_master_heartbeat_pause();
                }

                uint16_t ilen = expect_len;
                if (ilen > SPI_FORWARD_INNER_MAX) 
                {
                    local_acc_append(frm, expect_len);
                    return expect_len;
                }
                uint8_t inner[SPI_FORWARD_INNER_MAX] = {0};
                uint8_t slave_id = 0;
                memcpy(inner, frm, ilen);
                inner[ilen - 1] = bcc_xor(inner, (int)(ilen - 1));

                for (uint8_t i = 0; i < need_sync_slave_num; i++) {
                    slave_id = get_need_prog_sync_slave_id(i) - 1;
                    spi_forward_send_with_origin (slave_id, inner, ilen, is_uart ? RESP_UART : RESP_TCP, origin_id, true);
                }
                // master need process at all
                local_acc_append(frm, expect_len);
                return expect_len;
            }
            local_acc_append(frm, expect_len);
            return expect_len;
        }
        else {
            local_acc_append(frm, expect_len);
            return expect_len;
        }
    }

    // forward to slave: 2->slave1, 3->slave2 ...
    uint8_t slave_ord = (uint8_t)(recipe - 1); // 1..N
    //V1.1
    uint8_t available_slaves = router_available_slave_count();

    //if (slave_ord >= 1 && slave_ord <= SLAVE_SPI_MAX_NUM)
    //V1.1
    if (slave_ord >= 1 && slave_ord <= available_slaves)  
    {
        uint16_t ilen = expect_len;
        if (ilen > SPI_FORWARD_INNER_MAX) 
        {
            // too long for inner payload; fall back to local processing
            local_acc_append(frm, expect_len);
            return expect_len;
        }
        uint8_t inner[SPI_FORWARD_INNER_MAX] = {0};
        memcpy(inner, frm, ilen);
        inner[2] = 1; // rewrite recipe to 1 for slave
        inner[ilen - 1] = bcc_xor(inner, (int)(ilen - 1));
        // bus index 0..7 = slave_ord-1
        spi_forward_send_with_origin((uint8_t)(slave_ord - 1), inner, ilen, is_uart ? RESP_UART : RESP_TCP, origin_id, false);
        return expect_len;
    }

    // invalid mapping; pass to protocol
    local_acc_append(frm, expect_len);
    return expect_len;
}

/**
 * @brief  SPI packet assembly: build and send 
 * @note   core code for forwarding to slaves   
 * @param  slave_index0 : 0-7
 * @param  silent : true=从机不回复上位机(静默执行), false=正常回复
 */
static void spi_forward_send_with_origin(uint8_t slave_index0, const uint8_t *inner, uint16_t inner_len,
                                         resp_target_t target, uint8_t target_id, bool silent)
{
    if (inner_len > SPI_FORWARD_INNER_MAX) inner_len = SPI_FORWARD_INNER_MAX;
    if (slave_index0 >= SLAVE_SPI_MAX_NUM) return;

    //auto avoid conflict : notify param sync module
    notify_router_activity();

    // Save response route
    s_spi_resp_target[slave_index0] = target;
    s_spi_resp_silent[slave_index0] = silent ? 1 : 0;  // 保存静默标志
    if (target == RESP_UART) s_spi_resp_uart[slave_index0] = target_id;
    if (target == RESP_TCP)  s_spi_resp_sock[slave_index0] = target_id;


    uint8_t frame[SPI_FORWARD_FRAME_TOTAL];
    frame[0] = SPI_FRAME_HEAD;
    frame[1] = FRAME_TYPE_CMD;
    frame[2] = SPI_COMM_VERSION;
    frame[3] = (uint8_t)(slave_index0 + 1); // slave ID: 1..8
    for (uint8_t i = 0; i < SPI_FORWARD_INNER_MAX; i++) {
        frame[4 + i] = (i < inner_len) ? inner[i] : 0x00;
    }
    frame[SPI_FORWARD_FRAME_TOTAL - 1] = bcc_xor(frame, SPI_FORWARD_FRAME_TOTAL - 1);
    

    // 保存发送帧副本用于BCC错误重试
    memcpy(s_last_sent_frame[slave_index0], frame, SPI_FORWARD_FRAME_TOTAL);
    
    // 重置BCC重试计数器(新命令开始)
    s_bcc_retry_count[slave_index0] = 0;
    
    // ? 尝试立即发送
    spi_status_t spi_ret = bsp_spi_comm(slave_index0, frame, SPI_FORWARD_FRAME_TOTAL, 
                                        SPI_MASTER_DEFAULT_RESP_LEN, 
                                        SPI_MASTER_DEFAULT_DELAY_MS, 
                                        SPI_MASTER_DEFAULT_TIMEOUT_MS, 
                                        router_spi_cb);

    if (spi_ret == SPI_STATUS_OK) {
        #if DEBUG_MODE
           // LOG("SPI forward to slave %d success", slave_index0 + 1);
        #endif
    } 
    else if (spi_ret == SPI_STATUS_BUSY) {
        // SPI busy, push frame to retry queue
        if (retry_queue_push(slave_index0, frame, target, target_id, silent)) {
            LOG_W("SPI BUSY, command queued for slave %d", slave_index0 + 1);
        } 
        else {
            LOG_E("Retry queue full, command dropped for slave %d", slave_index0 + 1);
        }
    } 
    else {
        LOG_E("SPI forward to slave %d failed: %d", slave_index0 + 1, spi_ret);
    }
}



/**
 * @brief Core router processing function.
 * @note  used by both UART and TCP processing functions.
 * @param is_uart True if the data originated from UART, false if from TCP.
 * @param origin_id The UART number or TCP socket number where the data originated.
 * @param buf Pointer to the incoming data buffer.
 * @param len Length of the incoming data buffer.
 * @return None.
 */
static void router_core(bool is_uart, uint8_t origin_id, const uint8_t *buf, uint16_t len)
{
    local_acc_reset();
    uint16_t i = 0;
    while (i < len) 
    {
        uint8_t head = buf[i];
        if (head == HEAD_SINGLE) 
        {
            if (i + 6 > len) { 
                local_acc_append(&buf[i], (uint16_t)(len - i)); 
                break; 
            }
            route_single_frame(&buf[i], 6, is_uart, origin_id);
            i = (uint16_t)(i + 6);
        } 
        else if (head == HEAD_MULTI) 
        {
            if (i + 3 >= len) { 
                local_acc_append(&buf[i], (uint16_t)(len - i)); 
                break; 
            }
            uint8_t n = buf[i+2];
            uint16_t flen = (uint16_t)(4 + 3*n);
            if (i + flen > len) { 
                local_acc_append(&buf[i], (uint16_t)(len - i)); 
                break; 
            }
            route_multi_frame(&buf[i], flen, is_uart, origin_id);
            i = (uint16_t)(i + flen);
        }   
        else if (head == HEAD_PROG) 
        {
            uint16_t rem = (uint16_t)(len - i);
            uint16_t used = route_programmable_frame(&buf[i], rem, is_uart, origin_id);
            if (used == 0) { 
                local_acc_append(&buf[i], rem); 
                break; 
            }
            i = (uint16_t)(i + used);
        } 
        else 
        {
            // old protocol: pass to protocol stack as-is  
            local_acc_append(&buf[i], 1);
            i = (uint16_t)(i + 1);
        }
    }

	if (s_local_len > 0)
    {
		if (is_uart) {
			process_local_and_reply_uart(origin_id);
		}
	#if W5500_IS_ON
		else {
			process_local_and_reply_tcp(origin_id);
		}
	#endif
	}
}

uint8_t g_router_uart_rx_buf[50] = {0};

void  router_on_spi_reply_to_client(uint8_t slave_id, const uint8_t *data, uint16_t len)
{
    // 检查是否为静默模式
    if (s_spi_resp_silent[slave_id]) {
        // 静默模式: 不转发回复
        s_spi_resp_target[slave_id] = RESP_NONE;
        s_spi_resp_silent[slave_id] = 0;
        return;
    }

    memcpy(g_router_uart_rx_buf,   (uint8_t *)data + 4 , 21);
	if (len != 4)
		g_router_uart_rx_buf[len-1] = bcc_xor(g_router_uart_rx_buf, len-1);
    
	//memcpy(g_router_uart_rx_buf,  data , 26);
    resp_target_t t = s_spi_resp_target[slave_id];
    if (t == RESP_UART) {
		//memcpy(g_router_uart_rx_buf,  data , 26);
        //DMA_Usart_Send(s_spi_resp_uart[slave_id], (uint8_t *)data + 4, len);
		//DMA_Usart_Send(s_spi_resp_uart[slave_id], (uint8_t *)g_router_uart_rx_buf + 4, len);
        DMA_Usart_Send(s_spi_resp_uart[slave_id], g_router_uart_rx_buf, len);
    }
#if W5500_IS_ON
    else if (t == RESP_TCP) {
        //send(s_spi_resp_sock[slave_id], (uint8_t *) data + 4, len);
        send(s_spi_resp_sock[slave_id],  g_router_uart_rx_buf, len);
    }
#endif
    s_spi_resp_target[slave_id] = RESP_NONE;
    s_spi_resp_silent[slave_id] = 0;
}


/**
 * @brief: SPI receive callback  
 * @note    
 * @param: slave_id : 0..7 
 * @param: status : receive status
 * @param: response_data : received data pointer
 * @param: response_len : received data length
 * @return: None
 */
static void router_spi_cb(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t response_len)
{
    if (status == SPI_STATUS_OK && response_data && response_len) {
        router_on_spi_reply(slave_id, response_data, response_len);
    }
    if (status == SPI_STATUS_TIMEOUT) {
        LOG_E("SPI reply timeout from slave %d", slave_id + 1);
        g_spi_error_pending = 1;
    }

    router_check_sync_wait();
    // ? SPI事务完成后,尝试处理重试队列
    retry_queue_process();
}

uint8_t error_receive_buf[26] = {0};
void router_on_spi_reply(uint8_t slave_id, const uint8_t *data, uint16_t len)
{
    if (slave_id >= SLAVE_SPI_MAX_NUM) return;

    spi_payload_t *payload = (spi_payload_t *)(data);
    // BCC校验错误: 应用层重试机制(最多5次)
    if ( payload->frame_head != SPI_FRAME_HEAD || data[len - 1] != bcc_xor(data, len - 1)) {
        s_bcc_retry_count[slave_id]++;
        LOG_W("Slave received invalid data : 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X",
            data[0],data[1],data[2],data[3],data[4],data[5],
			data[6],data[7],data[8],data[9]);

        if (s_bcc_retry_count[slave_id] <= MAX_BCC_RETRY_COUNT) {
            LOG_W("Invalid BCC from slave %d, retry %d/%d", 
                  slave_id + 1, s_bcc_retry_count[slave_id], MAX_BCC_RETRY_COUNT);

            /*HAL_SPI_Abort(&hspi3);
            HAL_SPI_DMAStop(&hspi3);
			CLEAR_BIT(hspi3.Instance->CR2, SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
			volatile uint32_t tmp = hspi3.Instance->DR;
			tmp = hspi3.Instance->SR;*/
			bsp_spi_hardware_reinit();
            __NOP();  __NOP();  __NOP(); __NOP(); 
            __NOP();  __NOP();  __NOP(); __NOP(); 
            __NOP();  __NOP();  __NOP(); __NOP(); 
            spi_status_t ret = bsp_spi_comm(slave_id, 
                                            s_last_sent_frame[slave_id], 
                                            SPI_FORWARD_FRAME_TOTAL,
                                            SPI_MASTER_DEFAULT_RESP_LEN, 
                                            SPI_MASTER_DEFAULT_DELAY_MS, 
                                            SPI_MASTER_DEFAULT_TIMEOUT_MS, 
                                            router_spi_cb);
            
            if (ret == SPI_STATUS_BUSY) {
                //SPI bus busy , push buff into retry queue
                retry_queue_push(slave_id, s_last_sent_frame[slave_id], s_spi_resp_target[slave_id],
                                                        (s_spi_resp_target[slave_id] == RESP_UART)?
                                                s_spi_resp_uart[slave_id] : s_spi_resp_sock[slave_id],
                                                                        s_spi_resp_silent[slave_id]);
            }
            return;
        } 
        else {
            //V1.3  SYNC wait count decrement
            if (s_sync_wait_active && s_sync_wait_count > 0) {
                s_sync_wait_count--;
            }
			s_bcc_retry_count[slave_id] = 0; // 重置计数器
			uint8_t channel             = s_last_sent_frame[slave_id][6];   
			memcpy(error_receive_buf, s_last_sent_frame[slave_id], 26);
			error_receive_buf[1]        = 0x01;
			error_receive_buf[6]        = (slave_id + 1) * CHANNEL_NUM + channel;
			error_receive_buf[7]        = 0x04;  //

			router_on_spi_reply_to_client(slave_id, (uint8_t *)error_receive_buf, 4);
            LOG_E("BCC retry exhausted for slave %d, command failed,reply error", slave_id + 1);
			// g_spi_error_pending = 1;
            return;
        }
    }
    
    // BCC校验通过，重置重试计数器
    s_bcc_retry_count[slave_id] = 0;

    //V1.3  bcc success, so can SYNC wait count decrement
    if (s_sync_wait_active && s_sync_wait_count > 0) {
        s_sync_wait_count--;
    }

    switch (payload->frame_type)
    {
        case FRAME_TYPE_ACK:
            //if (data[4] == 0xCA || data[4] == 0xCB || data[4] == 0xCC) {
            if (data[4] == 0xCA || data[4] == 0xCB) {
                uint8_t cmd             = data[5];        
                uint8_t channel         = data[6];    
                bool is_broadcast_reply = false;

                // 检查是否为广播指令的回复
                //if (cmd == 0x41 || cmd == 0x46 || cmd == 0x47 || cmd == 0x43) {
                if (cmd == 0x46 || cmd == 0x47 || cmd == 0x43) {
                    // Set_mode, Set_RestoreFactorySettings, Set_DataSave, Set_Clean_Input_triggers_number
                    is_broadcast_reply = true;
                }
                else if ((cmd == 0x02 || cmd == 0x05 || cmd == 0x45) && (channel == 0)) {
                    // Set_Channel_Switch, Set_brightness_level, Set_softwareTrig (channel=0时广播)
                    is_broadcast_reply = true;
                }
                // 广播指令的回复不转发
                if (is_broadcast_reply) {
                    return;
                }

                uint8_t receive_length = 4;
                if (cmd == 0x61 || cmd == 0x62 || cmd == 0x63 || cmd == 0x64 || cmd == 0x65 ||
                    cmd == 0xA1 || cmd == 0xA2 || cmd == 0xA3 || cmd == 0xA4 || cmd == 0xA5 ||
                    cmd == 0xA6 || cmd == 0xA7 || cmd == 0xA8 ) {
                        receive_length = 6;
                }
				else if (cmd >= 0x81 && cmd <= 0x88){
					 receive_length = 6;
				}
                else if(cmd == 0x31){
                    receive_length = 21;
                }
                else if(cmd > Read_Programmable_data && cmd < Read_Prog_Command_Max ){
                    receive_length = 7;

                }
                else{
                    receive_length = 4;
                }

                // 非广播指令: 需要将从机本地通道号映射回全局通道号
                if (channel >= 1 && channel <= CHANNEL_NUM) {
                    // 计算全局通道号: (从机ID+1) * CHANNEL_NUM + 本地通道号
                    // slave_id: 0~7 对应 slave1~8
                    uint8_t global_channel = (slave_id + 1) * CHANNEL_NUM + channel;
                    
                    uint8_t receive_data[26] = {0};
                    memcpy(receive_data, data, len);
                    receive_data[6] = global_channel;  // 修改为全局通道号
                    router_on_spi_reply_to_client(slave_id, receive_data, receive_length);

                    if (cmd == Read_Brightness) {
                        uint16_t brightness = (data[7] <<8 ) | data[8];
                        set_slave_cache_brightness(slave_id+1,  channel-1, brightness);
                    }
              
                    if (cmd == Read_Pulse_width) {
                        uint16_t width = (data[7] <<8 ) | data[8];
                        set_slave_cache_strobe_width(slave_id + 1, channel-1, width);
                    }
                    if (cmd == Read_mode) {
                        uint8_t mode = data[8];
                        set_slave_cache_work_mode(slave_id + 1, mode);
                    }
                    if (cmd == Read_Digital_trigger_mode) {
                        uint8_t state = data[8];
                        set_slave_cache_normal_mode_state(slave_id + 1, state);
                    }


                } else {
                    // channel=0 或其他特殊情况，直接转发
                    router_on_spi_reply_to_client(slave_id, (uint8_t *)data, receive_length);
                }

            }
            else if (data[4] == 0xCC) {
                uint8_t cmd            = data[5];        
                uint8_t receive_length = 4;


                if(cmd == 0x31){
                    receive_length = 21;
                }
                else if(cmd >= Read_Prog_Total_steps && cmd < Read_Prog_Command_Max ){
                    receive_length = 7;
                }
                else{
                    receive_length = 4;
                }
  
                uint8_t receive_data[26] = {0};
                memcpy(receive_data, data, len);
                receive_data[6] = slave_id + 2 ;

				//LOG_I("CC receive data:0x%02X,0x%02X,0x%02X,0x%02X",receive_data[4],receive_data[5],receive_data[6],receive_data[7]);
                router_on_spi_reply_to_client(slave_id, receive_data, receive_length);
    
            }

            break; 

        case FRAME_TYPE_BCC_ERR:
        case FRAME_TYPE_HEAD_ERR:
        case FRAME_TYPE_LEN_ERR:
        case FRAME_TYPE_FUNC_ERR:
        case FRAME_TYPE_CMD_ERR:
        case FRAME_TYPE_SLAVE_ERR:
            LOG_E("SPI reply error frame type: %02X",payload->frame_type);
            g_spi_error_pending = 1;
            break;

        default:
            LOG_E("Unknown SPI reply frame type: %02X",payload->frame_type);
            g_spi_error_pending = 1;
            break;
    }

}

//******************************** Public APIs ********************************//

/**
 * @brief 初始化路由器模块 (在系统初始化时调用)
 */
void router_init(void)
{
    retry_queue_init();
    
    // 初始化BCC重试计数器
    for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++) {
        s_bcc_retry_count[i] = 0;
        memset(s_last_sent_frame[i], 0, SPI_FORWARD_FRAME_TOTAL);
    }
    
    LOG("Router initialized (BCC retry enabled: max %d attempts)", MAX_BCC_RETRY_COUNT);
}

/**
 * @brief 路由器定期处理函数 (在主循环中调用)
 * @note 用于处理重试队列中的待发送命令
 */
void router_handle(void)
{
    retry_queue_process();
    //V1.3 SYNC wait check
    router_check_sync_wait();
}

void router_process_uart(uint8_t uart_number, const uint8_t *buf, uint16_t len)
{
    router_core(true, uart_number, buf, len);
}

void router_process_tcp(uint8_t socket_sn, const uint8_t *buf, uint16_t len)
{
    router_core(false, socket_sn, buf, len);
}

void router_process_local(uint8_t socket_sn, const uint8_t *buf, uint16_t len)
{
    router_core(false, socket_sn, buf, len);
}
//******************************** Public APIs ********************************//


//******************************** Declaring ********************************//



