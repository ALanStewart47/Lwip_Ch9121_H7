#ifndef CH9121_PROTOCOL_H
#define CH9121_PROTOCOL_H

#include <stdint.h>

struct tcp_pcb;

void ch9121_protocol_init(void);
void ch9121_protocol_reset_session(void);
uint8_t ch9121_protocol_receive(const uint8_t *data, uint16_t length);
uint16_t ch9121_protocol_rx_available(void);
uint16_t ch9121_protocol_tx_pending(void);
void ch9121_protocol_process(struct tcp_pcb *pcb);
uint8_t ch9121_protocol_failed(void);

#endif
