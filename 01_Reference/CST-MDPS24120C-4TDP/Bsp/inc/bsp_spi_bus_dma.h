/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_spi_bus_dma.h
 *
 * @par dependencies
 * 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the spi bus master with DMA.
 *
 * Processing flow:
 * call directly.
 *
 * @version         V1.0    2025-09-08   ALan
 *                  V1.1    2025-09-11   ALan
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __BSP_SPI_BUS_DMA_H
#define __BSP_SPI_BUS_DMA_H

#include "main.h"
#include <stdbool.h>

typedef enum{
    SPI_BUS_IDLE = 0,
    SPI_BUS_BUSY = 1
}spi_busy_status_t;


typedef enum {
    SPI_COMM_INACTIVE = 0,  
    SPI_COMM_ACTIVE         
} spi_comm_active_t;

// SPI Master states
typedef enum {
    SPI_STATUS_OK = 0,
    SPI_STATUS_ERROR,
    SPI_STATUS_TIMEOUT,
    SPI_STATUS_BUSY,
    SPI_STATUS_INVALID_PARAM,
    SPI_STATUS_ABORTED
} spi_status_t;

// SPI Slave states
typedef enum {
    SPI_SLAVE_IDLE,            
    SPI_SLAVE_RECEIVING_CMD,    
    SPI_SLAVE_CMD_RECEIVED,     
    SPI_SLAVE_SENDING_RESP,     
    SPI_SLAVE_ERROR             
} spi_slave_state_t;


// Complete communication callback: called when send ¡ú wait ¡ú read response
typedef void (*spi_comm_callback_t)(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t response_len);

#define SPI_RECE_BUFFER_SIZE        30
#define SPI_SEND_BUFFER_SIZE        30
#define SPI_BUFFER_SIZE             30
#define SPI_DEFAULT_TIMEOUT_TICKS   100
#define NO_CURRENT_SLAVE            0xFF
#define SLAVE_SPI_MAX_NUM           8

//******************************** Declaring ********************************//
// core api : full master-slave communication 
// send data -> non-blocking wait -> send bytes to read response
spi_status_t bsp_spi_comm(uint8_t slave_id,
                          const uint8_t *request_data,
                          uint16_t request_len,
                          uint16_t response_len,
                          uint32_t delay_ms,
                          uint32_t timeout_ticks,
                          spi_comm_callback_t callback);
void bsp_InitSPIBus(void);
// Called periodically in the main loop to handle delays and the
void bsp_spi_process(void);  
spi_status_t bsp_spi_get_status(uint8_t slave_id); 
bool bsp_spi_is_bus_locked(void);  
spi_status_t bsp_spi_abort(uint8_t slave_id);  
// Called in timer interrupt 
void bsp_spi_on_timer_tick(void);
// SPI Slave mode APIs
spi_status_t bsp_spi_slave_start_receive(uint16_t expected_len, void (*callback)(uint8_t *data, uint16_t len));
spi_status_t bsp_spi_slave_stop_receive(void);
uint8_t bsp_spi_slave_get_state(void);

void bsp_spi_slave_set_tx_data(const uint8_t *data, uint16_t len);
spi_status_t bsp_spi_slave_ready_to_reply(const uint8_t *data, uint16_t len);
void bsp_spi_reset_consecutive_reinit_fail_count(void);
 void bsp_spi_hardware_reinit(void);
//******************************** Declaring ********************************//

#endif //__BSP_SPI_BUS_DMA_H


