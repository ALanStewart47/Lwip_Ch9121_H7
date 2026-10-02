#ifndef CH9121_SERVICE_H
#define CH9121_SERVICE_H

#include "lwip/err.h"
#include "lwip/netif.h"
#include <stdint.h>

typedef enum
{
  CH9121_STATUS_OK = 0,
  CH9121_STATUS_BIND_ERROR,
  CH9121_STATUS_BAD_PACKET,
  CH9121_STATUS_BAD_CONFIG,
  CH9121_STATUS_READ_ONLY_CHANGED,
  CH9121_STATUS_BUSY,
  CH9121_STATUS_SEND_ERROR,
  CH9121_STATUS_APPLY_ERROR
} ch9121_status_t;

typedef enum
{
  CH9121_INIT_IDLE = 0,
  CH9121_INIT_CONFIG,
  CH9121_INIT_UDP_ALLOC,
  CH9121_INIT_UDP_BIND,
  CH9121_INIT_DHCP_START,
  CH9121_INIT_READY
} ch9121_init_stage_t;

typedef struct
{
  volatile ch9121_status_t last_status;
  volatile unsigned long rx_packets;
  volatile unsigned long tx_packets;
  volatile unsigned long rejected_packets;
  volatile unsigned long tx_errors;
  volatile unsigned long apply_errors;
  volatile ch9121_init_stage_t init_stage;
  volatile err_t init_result;
  volatile err_t dhcp_start_result;
  volatile u16_t local_port;
  volatile unsigned long search_requests;
  volatile unsigned long get_requests;
  volatile unsigned long set_requests;
  volatile unsigned long accepted_sets;
  volatile unsigned long applied_sets;
  volatile u32_t last_rx_ms;
  volatile u32_t last_tx_ms;
  volatile err_t last_tx_result;
  volatile ch9121_status_t last_rejection;
  volatile u16_t last_sender_port;
  volatile uint8_t last_command;
  volatile uint8_t last_sender_ip[4];
  volatile uint8_t last_set_sender_ip[4];
  volatile uint8_t last_set_pc_mac[6];
} ch9121_diagnostics_t;

extern volatile ch9121_diagnostics_t g_ch9121_diagnostics;

err_t ch9121_service_init(struct netif *netif);
void ch9121_service_process(void);

#endif
