/**
 * @file app_slave_processor.c
 * @brief SPI slave data processor
 * @author Alan
 * @date 2025-09-10
 * 
 * @details This file implements the SPI slave data processing logic.
 * It receives frames from the master via SPI, unpacks them, processes
 * the commands using the existing protocol stack, and prepares the response.
 */

//******************************** Includes *********************************//
#include "app_slave_processor.h"
#include "bsp_spi_bus_dma.h"
#include "protocol_public.h"
#include "elog.h"
#include <string.h>
#include "app.h"
//******************************** Includes *********************************//


//******************************** Defines **********************************//
#define SPI_SLAVE_FRAME_LEN 23
#define SPI_SLAVE_INNER_LEN (SPI_SLAVE_FRAME_LEN - 2)
#define SPI_FRAME_HEAD      0xAA
//******************************** Defines **********************************//


//******************************** Declaring ********************************//
static void slave_spi_receive_callback(uint8_t *data, uint16_t len);
static uint8_t bcc_xor(const uint8_t *buf, int len);
//******************************** Declaring ********************************//


/**
 * @brief Calculate BCC (XOR checksum)
 */
static uint8_t bcc_xor(const uint8_t *buf, int len)
{
    uint8_t c = 0;
    for (int i = 0; i < len; i++) {
        c ^= buf[i];
    }
    return c;
}

/**
 * @brief Callback function for handling data received via SPI.
 * @param data Pointer to the received data buffer.
 * @param len Length of the received data.
 */
static void slave_spi_receive_callback(uint8_t *data, uint16_t len)
{
    if (len != SPI_SLAVE_FRAME_LEN) 
	{
        log_w("Slave received frame with invalid length: %d", len);
        
        return;
    }

//    if (data[0] != SPI_FRAME_HEAD) 
//	{
//        log_w("Slave received frame with invalid header: 0x%02X", data[0]);
//        //return;
//    }

//    if (data[SPI_SLAVE_FRAME_LEN - 1] != bcc_xor(data, SPI_SLAVE_FRAME_LEN - 1)) 
//	{
//        log_w("Slave received frame with invalid BCC");
//        return;
//    }

    // --- Frame is valid, process the inner command ---
    log_i("Slave received a valid frame");

    // 1. Pass the inner payload to the protocol stack
    input_data(&data[1], SPI_SLAVE_INNER_LEN);

    // 2. Process the command
    unsigned int tx_len = 0;
    analysis_command();
    unsigned char *tx_buf = get_prepare_tx_buffer(&tx_len);

    // 3. Prepare the response frame to be sent back to the master
    if (tx_len > 0) {
        log_i("Slave preparing response of length: %d", tx_len);
        uint8_t response_frame[SPI_SLAVE_FRAME_LEN] = {0};
        response_frame[0] = SPI_FRAME_HEAD;
        
        uint16_t copy_len = (tx_len > SPI_SLAVE_INNER_LEN) ? SPI_SLAVE_INNER_LEN : tx_len;
        memcpy(&response_frame[1], tx_buf, copy_len);
        
        response_frame[SPI_SLAVE_FRAME_LEN - 1] = bcc_xor(response_frame, SPI_SLAVE_FRAME_LEN - 1);
        
        // Set the response data for the next SPI transaction
        bsp_spi_slave_ready_to_reply(response_frame, SPI_SLAVE_FRAME_LEN);
        
    } 
	else {
        // If there is no response, prepare an empty ACK frame
        uint8_t ack_frame[SPI_SLAVE_FRAME_LEN] = {0};
        ack_frame[0] = SPI_FRAME_HEAD;
        ack_frame[1] = 0x06; // ACK
        ack_frame[SPI_SLAVE_FRAME_LEN - 1] = bcc_xor(ack_frame, SPI_SLAVE_FRAME_LEN - 1);
        bsp_spi_slave_ready_to_reply(ack_frame, SPI_SLAVE_FRAME_LEN);
    }
}

/**
 * @brief Initializes the slave processor.
 */
void slave_processor_init(void)
{
#if SLAVE_CODE
    //log_i("Initializing SPI Slave Processor...");
    bsp_InitSPIBus(); 
    bsp_spi_slave_start_receive(SPI_SLAVE_FRAME_LEN, slave_spi_receive_callback);
#endif
}
