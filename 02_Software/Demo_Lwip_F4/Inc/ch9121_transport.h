#ifndef CH9121_TRANSPORT_H
#define CH9121_TRANSPORT_H

#include "ch9121_port_config.h"
#include "lwip/err.h"
#include "lwip/netif.h"

typedef enum
{
  CH9121_TRANSPORT_DOWN = 0,
  CH9121_TRANSPORT_LISTEN,
  CH9121_TRANSPORT_CONNECTING,
  CH9121_TRANSPORT_CONNECTED,
  CH9121_TRANSPORT_UDP,
  CH9121_TRANSPORT_RESOLVING
} ch9121_transport_state_t;

typedef struct
{
  volatile ch9121_transport_state_t state;
  volatile err_t last_error;
  volatile u16_t local_port;
  volatile unsigned long connections;
  volatile unsigned long rx_bytes;
  volatile unsigned long tx_bytes;
  volatile unsigned long errors;
  volatile unsigned long retries;
  volatile uint8_t peer_ip[4];
} ch9121_transport_diagnostics_t;

extern volatile ch9121_transport_diagnostics_t g_ch9121_transport_diagnostics;

/* Network echo backend. No UART is selected or reconfigured here.
 * UART framing parameters are retained by the configuration service.
 */
void ch9121_transport_init(struct netif *netif, const ch9121_port_config_t *config);
void ch9121_transport_configure(const ch9121_port_config_t *config);
void ch9121_transport_process(void);

#endif
