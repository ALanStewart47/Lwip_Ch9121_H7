#ifndef CH9121_PORT_CONFIG_H
#define CH9121_PORT_CONFIG_H

#include <stdint.h>

#define CH9121_PORT_CONFIG_SIZE 65U
#define CH9121_DOMAIN_SIZE      33U
#define CH9121_MODE_TCP_SERVER  0U
#define CH9121_MODE_TCP_CLIENT  1U
#define CH9121_MODE_UDP_SERVER  2U
#define CH9121_MODE_UDP_CLIENT  3U

/* Relative to PortCfg[1], at configuration data offset 139.
 * WCH CH912XCFGDLL.H V1.1: little-endian integers, 65 bytes per port.
 * PortCfg[0] is port 2. The final byte is the Nagle enable flag.
 */
#define CH9121_PORT_MODE_OFFSET       2U
#define CH9121_PORT_RANDOM_OFFSET     3U
#define CH9121_PORT_LOCAL_OFFSET      4U
#define CH9121_PORT_DEST_IP_OFFSET    6U
#define CH9121_PORT_DEST_PORT_OFFSET  10U
#define CH9121_PORT_BAUD_OFFSET       12U
#define CH9121_PORT_DATA_OFFSET       16U
#define CH9121_PORT_STOP_OFFSET       17U
#define CH9121_PORT_PARITY_OFFSET     18U
#define CH9121_PORT_PHY_OFFSET        19U
#define CH9121_PORT_LENGTH_OFFSET     20U
#define CH9121_PORT_TIMEOUT_OFFSET    24U
#define CH9121_PORT_CLEAR_OFFSET      29U
#define CH9121_PORT_DNS_OFFSET        30U
#define CH9121_PORT_DOMAIN_OFFSET     31U
#define CH9121_PORT_NAGLE_OFFSET      64U

typedef struct
{
  uint8_t mode;
  uint8_t random_port;
  uint16_t local_port;
  uint8_t destination_ip[4];
  uint16_t destination_port;
  uint32_t baud_rate;
  uint8_t data_bits;
  uint8_t stop_bits;
  uint8_t parity;
  uint8_t close_on_link_loss;
  uint32_t rx_packet_length;
  uint32_t rx_packet_timeout; /* Units of 5 ms in NetModuleConfig V2.04. */
  uint8_t clear_on_connect;
  uint8_t use_domain;
  uint8_t domain[CH9121_DOMAIN_SIZE];
  uint8_t nagle; /* 0: TCP small packets enabled; 1: Nagle enabled. */
} ch9121_port_config_t;

void ch9121_port_defaults(ch9121_port_config_t *config);
void ch9121_port_encode(const ch9121_port_config_t *config,
                        uint8_t output[CH9121_PORT_CONFIG_SIZE]);
uint8_t ch9121_port_decode(const uint8_t input[CH9121_PORT_CONFIG_SIZE],
                           ch9121_port_config_t *config);
uint8_t ch9121_port_mutable_byte(uint32_t offset);
uint8_t ch9121_port_equal(const ch9121_port_config_t *first,
                          const ch9121_port_config_t *second);

#endif
