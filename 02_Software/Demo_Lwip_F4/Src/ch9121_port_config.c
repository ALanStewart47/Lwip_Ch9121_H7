#include "ch9121_port_config.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *bytes)
{
  return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
}

static uint32_t read_le32(const uint8_t *bytes)
{
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
         ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static void write_le16(uint8_t *bytes, uint16_t value)
{
  bytes[0] = (uint8_t)value;
  bytes[1] = (uint8_t)(value >> 8U);
}

static void write_le32(uint8_t *bytes, uint32_t value)
{
  bytes[0] = (uint8_t)value;
  bytes[1] = (uint8_t)(value >> 8U);
  bytes[2] = (uint8_t)(value >> 16U);
  bytes[3] = (uint8_t)(value >> 24U);
}

static uint8_t valid_destination(const uint8_t ip[4], uint8_t mode)
{
  if ((mode == CH9121_MODE_UDP_CLIENT) &&
      (ip[0] == 255U) && (ip[1] == 255U) && (ip[2] == 255U) && (ip[3] == 255U))
  {
    return 1U;
  }
  return (ip[0] != 0U) && (ip[0] != 127U) && (ip[0] < 224U);
}

static uint8_t valid_domain(const uint8_t domain[CH9121_DOMAIN_SIZE], uint8_t required)
{
  uint32_t i;
  uint8_t previous = 0U;

  for (i = 0U; i < CH9121_DOMAIN_SIZE; ++i)
  {
    uint8_t character = domain[i];
    if (character == 0U)
    {
      return (required == 0U) || ((i != 0U) && (previous != '.') && (previous != '-'));
    }
    if (((character < 'a') || (character > 'z')) &&
        ((character < 'A') || (character > 'Z')) &&
        ((character < '0') || (character > '9')) &&
        (character != '-') && (character != '.'))
    {
      return 0U;
    }
    if (((i == 0U) || (previous == '.')) && ((character == '.') || (character == '-')))
    {
      return 0U;
    }
    if ((character == '.') && (previous == '-'))
    {
      return 0U;
    }
    previous = character;
  }
  return 0U;
}

void ch9121_port_defaults(ch9121_port_config_t *config)
{
  memset(config, 0, sizeof(*config));
  config->mode = CH9121_MODE_TCP_SERVER;
  config->local_port = 6600U;
  config->destination_ip[0] = 192U;
  config->destination_ip[1] = 168U;
  config->destination_ip[2] = 1U;
  config->destination_ip[3] = 100U;
  config->destination_port = 1000U;
  config->baud_rate = 19200U;
  config->data_bits = 8U;
  config->stop_bits = 1U;
  config->parity = 4U;
  config->close_on_link_loss = 1U;
  config->rx_packet_length = 1024U;
  config->clear_on_connect = 1U;
  config->nagle = 1U;
}

uint8_t ch9121_port_mutable_byte(uint32_t offset)
{
  /* Index, enable flag and reconnect count stay read-only. */
  return ((offset >= CH9121_PORT_MODE_OFFSET) && (offset < 28U)) ||
         ((offset >= CH9121_PORT_CLEAR_OFFSET) && (offset < CH9121_PORT_CONFIG_SIZE));
}

void ch9121_port_encode(const ch9121_port_config_t *config,
                        uint8_t output[CH9121_PORT_CONFIG_SIZE])
{
  memset(output, 0, CH9121_PORT_CONFIG_SIZE);
  output[0] = 1U;
  output[1] = 1U;
  output[CH9121_PORT_MODE_OFFSET] = config->mode;
  output[CH9121_PORT_RANDOM_OFFSET] = config->random_port;
  write_le16(&output[CH9121_PORT_LOCAL_OFFSET], config->local_port);
  memcpy(&output[CH9121_PORT_DEST_IP_OFFSET], config->destination_ip, 4U);
  write_le16(&output[CH9121_PORT_DEST_PORT_OFFSET], config->destination_port);
  write_le32(&output[CH9121_PORT_BAUD_OFFSET], config->baud_rate);
  output[CH9121_PORT_DATA_OFFSET] = config->data_bits;
  output[CH9121_PORT_STOP_OFFSET] = config->stop_bits;
  output[CH9121_PORT_PARITY_OFFSET] = config->parity;
  output[CH9121_PORT_PHY_OFFSET] = config->close_on_link_loss;
  write_le32(&output[CH9121_PORT_LENGTH_OFFSET], config->rx_packet_length);
  write_le32(&output[CH9121_PORT_TIMEOUT_OFFSET], config->rx_packet_timeout);
  output[CH9121_PORT_CLEAR_OFFSET] = config->clear_on_connect;
  output[CH9121_PORT_DNS_OFFSET] = config->use_domain;
  memcpy(&output[CH9121_PORT_DOMAIN_OFFSET], config->domain, CH9121_DOMAIN_SIZE);
  output[CH9121_PORT_NAGLE_OFFSET] = config->nagle;
}

uint8_t ch9121_port_decode(const uint8_t input[CH9121_PORT_CONFIG_SIZE],
                           ch9121_port_config_t *config)
{
  uint8_t client_mode;

  memset(config, 0, sizeof(*config));
  config->mode = input[CH9121_PORT_MODE_OFFSET];
  config->random_port = input[CH9121_PORT_RANDOM_OFFSET];
  config->local_port = read_le16(&input[CH9121_PORT_LOCAL_OFFSET]);
  memcpy(config->destination_ip, &input[CH9121_PORT_DEST_IP_OFFSET], 4U);
  config->destination_port = read_le16(&input[CH9121_PORT_DEST_PORT_OFFSET]);
  config->baud_rate = read_le32(&input[CH9121_PORT_BAUD_OFFSET]);
  config->data_bits = input[CH9121_PORT_DATA_OFFSET];
  config->stop_bits = input[CH9121_PORT_STOP_OFFSET];
  config->parity = input[CH9121_PORT_PARITY_OFFSET];
  config->close_on_link_loss = input[CH9121_PORT_PHY_OFFSET];
  config->rx_packet_length = read_le32(&input[CH9121_PORT_LENGTH_OFFSET]);
  config->rx_packet_timeout = read_le32(&input[CH9121_PORT_TIMEOUT_OFFSET]);
  config->clear_on_connect = input[CH9121_PORT_CLEAR_OFFSET];
  config->use_domain = input[CH9121_PORT_DNS_OFFSET];
  memcpy(config->domain, &input[CH9121_PORT_DOMAIN_OFFSET], CH9121_DOMAIN_SIZE);
  config->nagle = input[CH9121_PORT_NAGLE_OFFSET];

  client_mode = (config->mode == CH9121_MODE_TCP_CLIENT) || (config->mode == CH9121_MODE_UDP_CLIENT);
  if ((config->mode > CH9121_MODE_UDP_CLIENT) || (config->random_port > 1U) ||
      (config->close_on_link_loss > 1U) || (config->clear_on_connect > 1U) ||
      (config->use_domain > 1U) || (config->nagle > 1U) ||
      (config->baud_rate < 300U) || (config->baud_rate > 921600U) ||
      (config->data_bits < 5U) || (config->data_bits > 8U) ||
      (config->stop_bits < 1U) || (config->stop_bits > 2U) || (config->parity > 4U) ||
      (config->rx_packet_length > 1024U) || (config->rx_packet_timeout > 0xFFFFFFFFUL / 5U))
  {
    return 0U;
  }
  if ((config->local_port == 0U) &&
      !((config->mode == CH9121_MODE_TCP_CLIENT) && (config->random_port != 0U)))
  {
    return 0U;
  }
  if ((config->mode >= CH9121_MODE_UDP_SERVER) && (config->local_port == 50000U))
  {
    return 0U; /* Already owned by the configuration service. */
  }
  if ((client_mode != 0U) &&
      ((config->destination_port == 0U) ||
       ((config->use_domain == 0U) && !valid_destination(config->destination_ip, config->mode))))
  {
    return 0U;
  }
  return valid_domain(config->domain, (uint8_t)((client_mode != 0U) && (config->use_domain != 0U)));
}

uint8_t ch9121_port_equal(const ch9121_port_config_t *first,
                          const ch9121_port_config_t *second)
{
  uint8_t first_wire[CH9121_PORT_CONFIG_SIZE];
  uint8_t second_wire[CH9121_PORT_CONFIG_SIZE];

  /* Compare serialized fields, not compiler padding in the RAM structure. */
  ch9121_port_encode(first, first_wire);
  ch9121_port_encode(second, second_wire);
  return (uint8_t)(memcmp(first_wire, second_wire, CH9121_PORT_CONFIG_SIZE) == 0);
}
