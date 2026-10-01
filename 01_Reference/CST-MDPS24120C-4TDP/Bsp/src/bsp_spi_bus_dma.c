/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_spi_bus_dma.c
 *
 * @par dependencies
 * - bsp_spi_bus_dma.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the spi communication master or slave with DMA or INT.
 *
 * Processing flow:
 * call directly.
 *
 * @version         V1.0    2025-09-08   ALan
 *                  V1.1    2025-09-11   ALan
 * 
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_spi_bus_dma.h"
#include <string.h>
#include <stdio.h>
#include "app.h"
#include "bsp_hcf4051.h"
#include "rtt_log.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define USE_SPI_DMA    
//#define USE_SPI_INT   
//#define USE_SPI_POLL  

#define SPI_SENT_MAX_RETRIES       3

extern SPI_HandleTypeDef hspi3;
extern DMA_HandleTypeDef hdma_spi3_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;

volatile uint8_t g_spi_error_pending;

typedef enum {
    TRANSFER_WAIT,
    TRANSFER_COMPLETE,
    TRANSFER_ERROR
} transfer_state_t;

typedef enum {
    COMM_IDLE       = 0 ,         
    COMM_PENDING        ,         // Pending (queued, waiting for bus to become free)
    COMM_SENDING        ,         
    COMM_WAITING        ,         // Waiting before reading response (delay gap)
    COMM_READING                  // Reading response (RX phase in progress)
} comm_phase_t;

typedef struct {
    uint8_t             active;                   
    comm_phase_t        phase;                    
    uint32_t            start_tick;     // for total timmeout        
    uint32_t            timeout_ticks;             
    uint32_t            recv_ticks;               
    uint32_t            recv_start_tick;          
    uint16_t            spi_send_len;             
    uint16_t            response_len;   // expected response length         
    spi_comm_callback_t callback;
    uint8_t             retry_count;                 
} spi_comm_context_t;

 spi_comm_context_t g_comm_ctx[SLAVE_SPI_MAX_NUM] = {0};

 // module internal time base : driven by TIM7
 static volatile uint32_t   g_spi_sched_tick         = 0;                   // add in TIM7 interrupt
 volatile uint8_t           g_spi_busy               = 0;
 volatile transfer_state_t  g_spi_phy_transfer_state = TRANSFER_WAIT;       // spi physical transfer state

 volatile uint8_t           g_current_slave          = NO_CURRENT_SLAVE;    // mask current slave id
 volatile bool              g_spi_bus_locked         = false;               // SPI总线互斥锁
 
 // 连续失败计数机制 - 防止无限重试导致系统卡顿
 static uint8_t             g_consecutive_reinit_fail_count = 0;            // 连续重新初始化失败计数
 #define SPI_MAX_CONSECUTIVE_REINIT_FAIL  10                                // 最大连续重新初始化失败次数
 
 //static 
 uint8_t             g_tx_buf[SLAVE_SPI_MAX_NUM][SPI_SEND_BUFFER_SIZE];  
 //static 
 uint8_t             g_rx_buf[SLAVE_SPI_MAX_NUM][SPI_RECE_BUFFER_SIZE];
 uint8_t             g_tx_back_buf[SLAVE_SPI_MAX_NUM][SPI_RECE_BUFFER_SIZE];

 #if SLAVE_CODE
    /*static*/ uint8_t      g_spiTxBuf      [SPI_BUFFER_SIZE];
    /*static*/ uint8_t      g_spiResponeBuf [SPI_BUFFER_SIZE];  
    /*static*/ uint8_t      g_spiRxBuf      [SPI_BUFFER_SIZE];
    
    typedef struct {
        spi_slave_state_t state;
        uint16_t expected_len;      // expected length of incoming command
        uint16_t response_len;      // ready to send response length
        uint16_t received_len;
        uint32_t last_receive_tick;
        void (*receive_callback)(uint8_t *data, uint16_t len);
    } spi_slave_context_t;
    
	volatile spi_slave_context_t g_slave_ctx = {0};
 #endif 

 static void finish_transfer(uint8_t slave_id);
 static inline uint32_t spi_now_tick(void);
	
 extern void MX_SPI3_Init(void);
 extern void MX_DMA_Init(void);
//******************************** Defines **********************************//


//******************************** Declaring ********************************//
/*static inline uint8_t bcc_xor(const uint8_t *buf, int len)
{
    uint8_t c = 0;
    for (int i = 0; i < len; i++) c ^= buf[i];
    return c;
}*/



/**
 * @brief:  slave spi bus init
 * @note
 * @param: void
 * @retval: void
 */
/*static void bsp_spi_slave_init(void)
{
#if SLAVE_CODE
	memset(g_spiTxBuf,      0, sizeof(g_spiTxBuf));
    memset(g_spiRxBuf,      0, sizeof(g_spiRxBuf));
    memset(g_spiResponeBuf, 0, sizeof(g_spiResponeBuf));
    
    g_slave_ctx.state               = SPI_SLAVE_IDLE;
    g_slave_ctx.expected_len        = 0;
    g_slave_ctx.received_len        = 0;
    g_slave_ctx.last_receive_tick   = 0;
    g_slave_ctx.receive_callback    = NULL;
	g_spi_busy                      = SPI_BUS_IDLE;
#endif
}*/

/**
 * @brief:  master spi bus init and  slave context init 
 * @note        
 * @param: void
 */
void bsp_InitSPIBus(void)
{	
#if MASTER_CODE
    g_spi_busy               = SPI_BUS_IDLE;
    g_current_slave          = NO_CURRENT_SLAVE;
    g_spi_phy_transfer_state = TRANSFER_WAIT;

    // init all slave contexts
    for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++) {
        memset(&g_comm_ctx[i], 0, sizeof(spi_comm_context_t));
        g_comm_ctx[i].phase  = COMM_IDLE;
    }
	LOG_I("SPI_BUS_Param reinit");
#endif
#if SLAVE_CODE
    //bsp_spi_slave_init();
#endif
}

/**
 * @brief:  get spi bus busy status
 * @retval: 0=idle, 1=busy
 */
uint8_t bsp_SpiBusBusy(void)
{
	return g_spi_busy;
}

 void bsp_spi_hardware_reinit(void)
{
    // 连续失败计数递增
    g_consecutive_reinit_fail_count++;
    LOG_W("SPI reinit count: %d/%d", g_consecutive_reinit_fail_count, SPI_MAX_CONSECUTIVE_REINIT_FAIL);
    
    // 检查是否超过最大失败次数
    if (g_consecutive_reinit_fail_count >= SPI_MAX_CONSECUTIVE_REINIT_FAIL) 
    {
        LOG_E("!!! SPI CRITICAL FAULT: %d consecutive reinit failures !!!", g_consecutive_reinit_fail_count);
        LOG_E("!!! System entering ERROR state - All SPI comm will be stopped !!!");
        
        // 触发系统错误状态
        set_light_alarm(STATUS_SPI_COMM_ERROR);  // 使用已定义的SPI通讯错误码
        return;  // 不再执行重新初始化,避免继续浪费CPU资源
    }
    
    HAL_SPI_Abort(&hspi3);
    #ifdef USE_SPI_DMA
        HAL_SPI_DMAStop(&hspi3);
    #endif

    HAL_DMA_Abort(&hdma_spi3_rx);
    HAL_DMA_Abort_IT(&hdma_spi3_rx);
    __HAL_DMA_DISABLE(hspi3.hdmarx);

    HAL_DMA_Abort(&hdma_spi3_tx);
    HAL_DMA_Abort_IT(&hdma_spi3_tx);
    __HAL_DMA_DISABLE(hspi3.hdmatx);

    CLEAR_BIT(hspi3.Instance->CR2, SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

    __HAL_SPI_DISABLE(&hspi3);

    __HAL_SPI_CLEAR_OVRFLAG(&hspi3);
    __HAL_SPI_CLEAR_CRCERRFLAG(&hspi3);
    __HAL_SPI_CLEAR_FREFLAG(&hspi3);
    __HAL_SPI_CLEAR_MODFFLAG(&hspi3);

    __HAL_SPI_ENABLE(&hspi3);

    hspi3.State = HAL_SPI_STATE_READY;
    HAL_SPI_MspDeInit(&hspi3);
    HAL_SPI_MspInit(&hspi3);
    MX_SPI3_Init();
    MX_DMA_Init();
	
	LOG("hardware reinit spi and dma");
}


//******************************** Hal Interrupt Callback ********************************//

/**
 * @brief:  HAL SPI CALLBACKS FUNCTIONS  
 * @param:  hspi3
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == &hspi3) {
        g_spi_phy_transfer_state = TRANSFER_COMPLETE;

    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == &hspi3) {
        g_spi_phy_transfer_state = TRANSFER_ERROR;
		g_spi_error_pending = 1;
        LOG_E("[Interrupt] Transfer error callback\r\n");
    }
}

//******************************** Hal Interrupt Callback ********************************//


//******************************** spi connect function time base and timeout check  ********************************// 
static inline uint32_t spi_now_tick(void) 
{ 
    return g_spi_sched_tick; 
}

//called by TIM7 interrupt 
void bsp_spi_on_timer_tick(void){
    g_spi_sched_tick++;
}

static uint8_t is_timeout(uint32_t start_tick, uint32_t timeout_ticks)
{
    if (timeout_ticks == 0) 
        timeout_ticks = SPI_DEFAULT_TIMEOUT_TICKS;

    return (spi_now_tick() - start_tick) >= timeout_ticks;
}

void bsp_spi_reset_consecutive_reinit_fail_count(void)
{
    g_consecutive_reinit_fail_count = 0;
}
//------  spi connect function time base and timeout check  ------// 


/**
 * @brief   start physical transfer of SPI bus        
 * @param   slave_id : slave index 0..7,
 *          len : transfer length
 * @retval  spi_status_t
 **/
static spi_status_t start_transfer(uint8_t slave_id, uint16_t len)
{  
    //SPI bus is busy
    g_spi_busy               = SPI_BUS_BUSY ;
    g_current_slave          = slave_id     ;
    g_spi_phy_transfer_state = TRANSFER_WAIT;
    //ness gpio set low to select slave 
    bsp_hcf4051_set_channel(slave_id);

    // 拆除“核弹”：不再因为重试而执行 reinit
    // if (g_comm_ctx[slave_id].retry_count > 0)
    // {
    //     bsp_spi_hardware_reinit();
    // }

    uint32_t timeout = 10000;  // 10000≈1ms 
    while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_BSY) && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        LOG_E("SPI BSY timeout before transfer slave %d", slave_id);
        finish_transfer(slave_id); // 优雅地结束本次传输
        return SPI_STATUS_ERROR;   // 返回错误，让上层处理
    }
    __HAL_SPI_DISABLE(&hspi3);

    __NOP(); __NOP(); __NOP(); __NOP();

        if (HAL_SPI_TransmitReceive_DMA(&hspi3, g_tx_buf[slave_id], g_rx_buf[slave_id], len) != HAL_OK) 
        {
            finish_transfer(slave_id);
            LOG_E("Failed to transfer slave %d", slave_id);
            return SPI_STATUS_ERROR;
        }
    return SPI_STATUS_OK;
}

/**
 * @brief:  finish transfer and release the bus
 * @param:  slave_id : slave index 0..7
 * @retval: void
 */
void finish_transfer(uint8_t slave_id)
{
#if MASTER_CODE
    HAL_SPI_Abort(&hspi3);
    HAL_SPI_DMAStop(&hspi3);
    __HAL_SPI_DISABLE(&hspi3);
#endif
    g_spi_busy      = SPI_BUS_IDLE;
    g_current_slave = NO_CURRENT_SLAVE;
}


/**
 * @brief: start SPI slave continuous receive
 * @note:  In slave mode , SPI will continuously be in receive state, waiting for the master to send data
 * @param: expected_len : expected length of incoming data
 *         callback :  receive complete callback
 * @retval: spi_status_t
 */
spi_status_t bsp_spi_slave_start_receive(uint16_t expected_len, void (*callback)(uint8_t *data, uint16_t len))
{
    if (expected_len > SPI_BUFFER_SIZE) {
        return SPI_STATUS_INVALID_PARAM;
    }
    
    if (g_slave_ctx.state != SPI_SLAVE_IDLE) {
        LOG_W("SPI slave is not idle, current state: %d", g_slave_ctx.state);
        return SPI_STATUS_BUSY;
    }

    //10/13 ；new code - 添加超时保护避免死循环
    uint32_t timeout = 10000;  // 10000≈1ms
    while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_BSY) && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        LOG_E("SPI BSY timeout in slave mode");
        bsp_spi_hardware_reinit();
    }
    __HAL_SPI_DISABLE(&hspi3);

    // clear buffers 
    memset(g_spiRxBuf, 0, sizeof(g_spiRxBuf));
    memset(g_spiTxBuf, 0xf1, sizeof(g_spiTxBuf));

    g_slave_ctx.state             = SPI_SLAVE_RECEIVING_CMD;
    g_slave_ctx.expected_len      = expected_len;
    g_slave_ctx.received_len      = 0;
    g_slave_ctx.last_receive_tick = spi_now_tick();
    g_slave_ctx.receive_callback  = callback;

    g_spi_busy                    = SPI_BUS_BUSY;
    g_spi_phy_transfer_state      = TRANSFER_WAIT;


    if (HAL_SPI_TransmitReceive_DMA(&hspi3, g_spiTxBuf, g_spiRxBuf, expected_len) != HAL_OK) 
    {
        g_slave_ctx.state   = SPI_SLAVE_IDLE;
        g_spi_busy          = SPI_BUS_IDLE;
        //log_e("Failed to start SPI slave receive");
        return SPI_STATUS_ERROR;
    }

    //log_i("SPI slave started receiving, expecting %d bytes", expected_len);
    return SPI_STATUS_OK;
}

/**
 * @brief: Stop SPI slave receiving
 * @param: void
 * @retval: spi_status_t
 */
spi_status_t bsp_spi_slave_stop_receive(void)
{
    if (g_slave_ctx.state == SPI_SLAVE_RECEIVING_CMD) {

        HAL_SPI_Abort(&hspi3);
        
        g_slave_ctx.state = SPI_SLAVE_IDLE;
        g_spi_busy = SPI_BUS_IDLE;
        g_spi_phy_transfer_state = TRANSFER_WAIT;
        
        //log_i("SPI slave receive stopped");
    }
    
    return SPI_STATUS_OK;
}

/**
 * @brief: get SPI slave state
 * @param: void
 * @retval: spi_slave_state_t
 */
uint8_t bsp_spi_slave_get_state(void)
{
    return g_slave_ctx.state;
}

/**
 * @brief: restart SPI slave receive for continuous reception
 * @note:  receive complete , auto restart
 * @param: void
 * @retval: spi_status_t
 */
/*static spi_status_t restart_slave_receive(void)
{
    if (g_slave_ctx.expected_len == 0) {
        return SPI_STATUS_INVALID_PARAM;
    }

    //memset(g_spiRxBuf, 0, sizeof(g_spiRxBuf));

    g_slave_ctx.state             = SPI_SLAVE_RECEIVING_CMD;
    g_slave_ctx.received_len      = 0;
    g_slave_ctx.last_receive_tick = spi_now_tick();
    
    g_spi_busy                    = SPI_BUS_BUSY;
    g_spi_phy_transfer_state      = TRANSFER_WAIT;

#ifdef USE_SPI_DMA
    if (HAL_SPI_TransmitReceive_DMA(&hspi3, g_spiTxBuf, g_spiRxBuf, g_slave_ctx.expected_len) != HAL_OK)
#elif defined(USE_SPI_INT)
    if (HAL_SPI_TransmitReceive_IT(&hspi3, g_spiTxBuf, g_spiRxBuf, g_slave_ctx.expected_len) != HAL_OK)
#elif defined(USE_SPI_POLL)
    if (HAL_SPI_TransmitReceive(&hspi3, g_spiTxBuf, g_spiRxBuf, g_slave_ctx.expected_len, 5000) != HAL_OK)
#endif
    {
        g_slave_ctx.state   = SPI_SLAVE_IDLE;
        g_spi_busy          = SPI_BUS_IDLE;
        //log_e("Failed to restart SPI slave receive");
        return SPI_STATUS_ERROR;
    }
    return SPI_STATUS_OK;
}*/

/**
 * @brief: set tx data for next transfer
 * @note  
 * @param data pointer to data to send
 * @param len  data length 
 */
void bsp_spi_slave_set_tx_data(const uint8_t *data, uint16_t len)
{
#if SLAVE_CODE
    if (len > SPI_BUFFER_SIZE) {
        len = SPI_BUFFER_SIZE;
    }
    memcpy(g_spiTxBuf, data, len);
#endif
}

/**
 * @brief: prepare to reply data to master
 * @note:  after receiving command, call this function to prepare response data
 * @param: data pointer to response data
 *         len  length of response data
 * @retval: spi_status_t
 */
spi_status_t bsp_spi_slave_ready_to_reply(const uint8_t *data, uint16_t len)
{
#if SLAVE_CODE
    if (g_slave_ctx.state != SPI_SLAVE_CMD_RECEIVED) {
        //log_w("Slave is not in CMD_RECEIVED state, cannot prepare reply.");
        return SPI_STATUS_ERROR;
    }

    if (len > SPI_BUFFER_SIZE) {
        //log_e("Reply data length (%d) exceeds buffer size.", len);
        return SPI_STATUS_INVALID_PARAM;
    }
    // copy response data to response buffer
    if (data && len > 0) {
        memcpy(g_spiResponeBuf, data, len);
    }
    g_slave_ctx.response_len = len;

#ifdef USE_SPI_DMA
    if (HAL_SPI_TransmitReceive_DMA(&hspi3, g_spiResponeBuf, g_spiRxBuf, g_slave_ctx.response_len) != HAL_OK)
#elif defined(USE_SPI_INT)
    if (HAL_SPI_TransmitReceive_IT(&hspi3, g_spiResponeBuf, g_spiRxBuf, g_slave_ctx.response_len) != HAL_OK)
#endif
    {
        //log_e("Failed to start SPI for sending reply.");
        g_slave_ctx.state   = SPI_SLAVE_ERROR;
        return SPI_STATUS_ERROR;
    }

    // into sending response state
    g_slave_ctx.state        = SPI_SLAVE_SENDING_RESP;
    g_spi_busy               = SPI_BUS_BUSY;
    g_spi_phy_transfer_state = TRANSFER_WAIT;

    return SPI_STATUS_OK;
#else
    return SPI_STATUS_OK;
#endif
}


//receive_ticks is in ticks (1 tick = 100us)
/**
 * @brief: complete spi master-slave communication
 * @note        
 * @param: slave_id : slave index 0..7,
 *        send_data : data to send pointer, can be NULL if no data to send
 *        send_len  : length of data to send, can be 0 if no data to send
 *      receive_len : expected response length, can be 0 if no response needed
 *    receive_ticks : delay ticks after sending request before reading response, can be 0 if no response needed
 *    timeout_ticks : overall timeout ticks for the whole operation, if 0 use default value
 *         callback : callback function when the whole operation is complete, can be NULL if no callback needed
 * @retval: spi_status_t
 */
spi_status_t bsp_spi_comm(uint8_t               slave_id,
                          const uint8_t         *send_data,
                          uint16_t              send_len,
                          uint16_t              receive_len,
                          uint32_t              receive_ticks,
                          uint32_t              timeout_ticks,
                          spi_comm_callback_t   callback)
{
#if MASTER_CODE
    extern uint8_t get_light_alarm(void);
    uint8_t alarm_status = get_light_alarm();
    if (alarm_status == STATUS_SPI_COMM_ERROR) {
        // system is in SPI communication error state, reject all SPI communications
        LOG_E("SPI communication rejected due to system error state");
        if (callback) {
            callback(slave_id, SPI_STATUS_ERROR, NULL, 0);
        }
        return SPI_STATUS_ERROR;  
    }
    
    if (g_spi_bus_locked) {
        return SPI_STATUS_BUSY;
    }
    if (slave_id    >=  SLAVE_SPI_MAX_NUM) 
        return SPI_STATUS_INVALID_PARAM;
    if (send_len    >   SPI_SEND_BUFFER_SIZE) 
        return SPI_STATUS_INVALID_PARAM;
    if (receive_len >   SPI_RECE_BUFFER_SIZE) 
        return SPI_STATUS_INVALID_PARAM;

    spi_comm_context_t *ctx = &g_comm_ctx[slave_id];
    
    // check slave is busy now 
    if (ctx->active) {
        LOG_W("Slave %d is busy", slave_id);
        return SPI_STATUS_BUSY;
    }

    // Lock the spi bus
    g_spi_bus_locked = true;

    // copy send data to global slave[x] buffer
    if (send_data && send_len > 0) {
        memcpy(g_tx_buf[slave_id], send_data, send_len);
        memcpy(g_tx_back_buf[slave_id], send_data, send_len);
    }

    ctx->active         = SPI_COMM_ACTIVE;
    ctx->timeout_ticks  = (timeout_ticks == 0) ? SPI_DEFAULT_TIMEOUT_TICKS : timeout_ticks;  
    ctx->spi_send_len   = send_len;
    ctx->response_len   = receive_len;
    ctx->recv_ticks     = receive_ticks;    
    ctx->callback       = callback;
    ctx->start_tick     = spi_now_tick();  
    ctx->retry_count    = 0;

    //If the bus is idle, start immediately; 
    //otherwise, set to PENDING state and wait for scheduling."
    if (!g_spi_busy && send_len > 0) 
	{
        ctx->phase = COMM_SENDING;
        if (start_transfer(slave_id, send_len) != SPI_STATUS_OK) {
            ctx->active         = 0;
            ctx->phase          = COMM_IDLE;
            g_spi_bus_locked    = false; // release lock
            LOG_E("Failed to transfer by spi physical layer");
            return SPI_STATUS_ERROR;
        }
    }
    // bus is idle , set to WAITING state 
	else if (!g_spi_busy && send_len == 0 && receive_len > 0) 
	{
        ctx->phase = COMM_WAITING;
        ctx->recv_start_tick = spi_now_tick();
        LOG("No send data, entering wait phase directly");
    }
    else // bus is busy , set to PENDING state 
	{
        // if spi is busy, release the lock as the send is not started yet
        g_spi_bus_locked = false;
        ctx->phase = COMM_PENDING;
        LOG("SPI is busy, task queued as PENDING\r\n");
    }

#endif
    return SPI_STATUS_OK; 
}

// get spi slave is busy or not
spi_status_t bsp_spi_get_status(uint8_t slave_id)
{
    if (slave_id >= SLAVE_SPI_MAX_NUM) 
        return SPI_STATUS_INVALID_PARAM;
    
    spi_comm_context_t *ctx = &g_comm_ctx[slave_id];
    
    if (!ctx->active) 
        return SPI_STATUS_OK; 

    return SPI_STATUS_BUSY; 
}

/**
 * @brief: Check if SPI bus is locked (read-only access)
 * @note   用于提前判断SPI总线是否可用,避免无效的组包操作
 * @param: None
 * @retval: true=总线被锁定(BUSY), false=总线空闲(可用)
 */
bool bsp_spi_is_bus_locked(void)
{
    return g_spi_bus_locked;
}


/**
 * @brief: abort current communication with the specified slave
 * @note    
 * @param: slave_id : slave index 0..7
 * @retval: spi_status_t
 */
spi_status_t bsp_spi_abort(uint8_t slave_id)
{
    if (slave_id >= SLAVE_SPI_MAX_NUM) return SPI_STATUS_INVALID_PARAM;
    spi_comm_context_t *ctx = &g_comm_ctx[slave_id];
    
    if (ctx->active) 
    {
        if (g_current_slave == slave_id && g_spi_busy)
        {
            HAL_SPI_Abort(&hspi3);
            HAL_SPI_DMAStop(&hspi3);
            finish_transfer(slave_id);
            LOG_W("Hardware aborted for slave %d", slave_id);
        }
        // Callback to notify the upper layer
        if (ctx->callback) {
            ctx->callback(slave_id, SPI_STATUS_ABORTED, NULL, 0);
        }
        // clear context
        ctx->active = 0;
        ctx->phase = COMM_IDLE;
        g_spi_bus_locked = false; 
       // log_i("Communication aborted for slave %d", slave_id);
    }
    return SPI_STATUS_OK;
}



/**
 * @brief : spi bus process function, should be called periodically
 * @note
 * @param : void
 * @retval: void
 */
void bsp_spi_process(void)
{
//1. check all slave commnunication is timeout or not
    for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++) 
    {
        spi_comm_context_t *ctx = &g_comm_ctx[i];
        //spi communication timeout
        if (ctx->active && is_timeout(ctx->start_tick, ctx->timeout_ticks)) 
        {
            //if the timeout slave is in physical transfer, abort it
            if (g_current_slave == i && g_spi_busy) 
            {
                HAL_SPI_Abort(&hspi3);
                #ifdef USE_SPI_DMA
                    HAL_SPI_DMAStop(&hspi3);
                #endif
                finish_transfer(i);         //free the bus
                LOG_E("ERROR: Total timeout for slave %d, transfer aborted\r\n", i);
            }
            
            // notify upper layer
            if (ctx->callback) {
                ctx->callback(i, SPI_STATUS_TIMEOUT, NULL, 0);
            }
        
            ctx->active = 0;
            ctx->phase = COMM_IDLE;
            g_spi_bus_locked = false; // 释放锁
        }
    }

// 2. handle current slave physical transfer 
    if (g_spi_busy == SPI_BUS_BUSY  &&  g_current_slave < SLAVE_SPI_MAX_NUM) 
    {
        spi_comm_context_t *ctx = &g_comm_ctx[g_current_slave];
        
        //check transfer by physical complete
        if (g_spi_phy_transfer_state == TRANSFER_COMPLETE) 
		{
            uint8_t completed_slave = g_current_slave;
            //sent and relesae spi bus
            finish_transfer(g_current_slave);
            
            if (ctx->phase == COMM_SENDING)                     //SPI SENT PHASE
            {
                // send complete, enter waiting receive phase(enter next judgment function)
                ctx->phase = COMM_WAITING;
                ctx->recv_start_tick = spi_now_tick();
            } 
            else if (ctx->phase == COMM_READING)                //SPI READ PHASE
            {
                /*if (g_rx_buf[completed_slave][0] != 0xAA ){
                    if (ctx->retry_count < SPI_SENT_MAX_RETRIES){
                        ctx->retry_count++;
                        LOG_E("[spi_process]Invalid HEAD, retry %d",ctx->retry_count);
						LOG_E("Slave received frame with invalid header : 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X, ",
								g_rx_buf[completed_slave][0],g_rx_buf[completed_slave][1],g_rx_buf[completed_slave][2],g_rx_buf[completed_slave][3],g_rx_buf[completed_slave][4],g_rx_buf[completed_slave][5],g_rx_buf[completed_slave][6],g_rx_buf[completed_slave][7],g_rx_buf[completed_slave][8],g_rx_buf[completed_slave][9],g_rx_buf[completed_slave][10],g_rx_buf[completed_slave][11],g_rx_buf[completed_slave][12],g_rx_buf[completed_slave][13],g_rx_buf[completed_slave][14],g_rx_buf[completed_slave][15],g_rx_buf[completed_slave][16],g_rx_buf[completed_slave][17],g_rx_buf[completed_slave][18],g_rx_buf[completed_slave][19],g_rx_buf[completed_slave][20],g_rx_buf[completed_slave][21],g_rx_buf[completed_slave][22]);

						
                        ctx->phase = COMM_PENDING;
						ctx->start_tick = spi_now_tick();
                    }
                    else{
                        LOG_E("[spi_process]Retry count exceed, report error,Invalid HEAD, retry %d",ctx->retry_count);
                        if (ctx->callback) {
                            ctx->callback(completed_slave, SPI_STATUS_ERROR, g_rx_buf[completed_slave], ctx->response_len);
                        }
                        ctx->active = 0;
                        ctx->phase = COMM_IDLE;
						g_spi_error_pending = 1;
                        g_spi_bus_locked = false; // 释放锁
                    }
                }//if(g_rx_buf[completed_slave][0] != 0xAA)
                else{*/
                    // 帧头验证通过，数据合法，清零连续失败计数器
                    if (g_consecutive_reinit_fail_count > 0) {
                        //LOG_I("SPI comm recovered (valid frame), resetting fail count (was %d)", g_consecutive_reinit_fail_count);
                        g_consecutive_reinit_fail_count = 0;
                    }
                    
                    //response is done , callback complete and clear context
                    if (ctx->callback) {
                        ctx->callback(completed_slave, SPI_STATUS_OK, g_rx_buf[completed_slave], ctx->response_len);
                    }
                    ctx->active = 0;
                    ctx->phase = COMM_IDLE;
                    g_spi_bus_locked = false; // 释放锁
                    //LOG("received for slave%d, communication complete", completed_slave);
                //}//
            }
            g_spi_phy_transfer_state = TRANSFER_WAIT; 
        }
        //physical transfer error
        else if (g_spi_phy_transfer_state == TRANSFER_ERROR) 
		{
            uint8_t error_slave = g_current_slave; 
            finish_transfer(g_current_slave);
            LOG_E("Physical transfer error for slave %d", error_slave);
            
            if (ctx->callback) {
                ctx->callback(error_slave, SPI_STATUS_ERROR, NULL, 0);
            }
            ctx->active = 0;
            ctx->phase = COMM_IDLE;
            g_spi_bus_locked = false; // 释放锁
            g_spi_phy_transfer_state = TRANSFER_WAIT; 
        }
    }//if (g_spi_busy == SPI_BUS_BUSY  &&  g_current_slave < SLAVE_SPI_MAX_NUM) 
    
// 3. bus is idle , start to response and try to schedule pending tasks
    if (g_spi_busy == SPI_BUS_IDLE)
    {
        // firstly schedule the waiting slaves (ready to read response)
        for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++) 
        {
            spi_comm_context_t *ctx = &g_comm_ctx[i];
            //start to read response after waiting ticks 
            if (ctx->active && ctx->phase == COMM_WAITING) 
            {
                // check receive tick is  timeout or not
                if ((spi_now_tick() - ctx->recv_start_tick) >= ctx->recv_ticks) 
                {
                    if (ctx->response_len > 0) 
                    {
                        // ready to read response : fill 0xF1 to generate read request
                        memset(g_tx_buf[i], 0xF1, ctx->response_len);
                        ctx->phase = COMM_READING;      
                        
                        // start to read response
                        if (start_transfer(i, ctx->response_len) != SPI_STATUS_OK) 
                        {
                            LOG_E("ERROR: Failed to start response read for slave %d\r\n", i);
                            if (ctx->callback) {
                                ctx->callback(i, SPI_STATUS_ERROR, NULL, 0);
                            }
                            ctx->active = 0;
                            ctx->phase = COMM_IDLE;
                            g_spi_bus_locked = false; // 释放锁
                        }
                    } 
                    else 
                    {
                        //No response expected , complete the communication directly
                        LOG_W("No response expected for slave %d, communication complete\r\n", i);
                        if (ctx->callback) {
                            ctx->callback(i, SPI_STATUS_INVALID_PARAM, NULL, 0);
                        }
                        ctx->active = 0;
                        ctx->phase = COMM_IDLE;
                        g_spi_bus_locked = false; // 释放锁
                    }
                    return; //evertime start one only to avoid conflict
                }
            }
        }

        // secondly schedule the pending slaves (ready to send request)
        for (uint8_t i = 0; i < SLAVE_SPI_MAX_NUM; i++)
        {
            spi_comm_context_t *ctx = &g_comm_ctx[i];
            if (ctx->active && ctx->phase == COMM_PENDING)
            {
                // 尝试为 PENDING 任务获取锁
                /*if (!g_spi_bus_locked) {
                    g_spi_bus_locked = true;
                } else {
                    continue; // 如果锁已被其他任务（如 WAITING 转 READING）获取，则跳过
                }*/

                if (ctx->spi_send_len > 0) 
                {
                    ctx->phase = COMM_READING;
                    ctx->recv_start_tick = spi_now_tick();
					//HAL_Delay(1);
					 memcpy(g_tx_buf[i], g_tx_back_buf[i], 26);
        
                    LOG_I("start pending request");
                    if (start_transfer(i, ctx->spi_send_len) != SPI_STATUS_OK)
                    {
                        LOG_E("ERROR: Failed to start pending request for slave %d", i);

                        if (ctx->callback){
                            ctx->callback(i, SPI_STATUS_ERROR, NULL, 0);
                        }
                        ctx->active = 0;
                        ctx->phase = COMM_IDLE;
                        g_spi_bus_locked = false; // 释放锁
                    }
                } 
                else 
                {
                    // no request to send, enter waiting phase directly
                    ctx->phase = COMM_WAITING;
                    ctx->recv_start_tick = spi_now_tick();
                    LOG_W("Pending task for slave %d has no request, entering wait phase\r\n", i);
                }
                return; //evertime start one only to avoid conflict
            }
        }
    }

    // spi error occurred
    if (g_spi_error_pending == 1 )
    {
        g_spi_error_pending = 0;

        // 仅在发生硬件错误时才执行 reinit
        bsp_spi_hardware_reinit();
    
        //LOG_E("SPI error occurred,receive again");
        
        if(HAL_SPI_GetState(&hspi3) == HAL_SPI_STATE_READY)
        {
            if (g_slave_ctx.expected_len > 0 && g_slave_ctx.receive_callback) 
            {
                HAL_Delay(1);
                bsp_InitSPIBus();                 
            }
        }
    }
}



//******************************** Declaring ********************************//


