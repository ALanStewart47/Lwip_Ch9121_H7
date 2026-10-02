#include "ch9121_service.h"
#include "ch9121_port_config.h"
#include "ch9121_transport.h"

#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/prot/ethernet.h"
#include "lwip/udp.h"
#include "lwip/sys.h"

#include <string.h>

#define CH9121_DEVICE_PORT       50000U
#define CH9121_CLIENT_PORT       60000U
#define CH9121_PACKET_SIZE       285U
#define CH9121_HEADER_SIZE       30U
#define CH9121_DATA_SIZE         255U
#define CH9121_CONFIG_SIZE       204U
#define CH9121_CONFIG_PAD_SIZE   51U
#define CH9121_NAME_LENGTH       21U

#define CH9121_CMD_SET           0x01U
#define CH9121_CMD_GET           0x02U
#define CH9121_CMD_SEARCH        0x04U
#define CH9121_ACK_SET           0x81U
#define CH9121_ACK_GET           0x82U
#define CH9121_ACK_SEARCH        0x84U
#define CH9121_NAK_SET           0xC1U

#define HWCFG_NAME_OFFSET        5U
#define HWCFG_MAC_OFFSET         26U
#define HWCFG_IP_OFFSET          32U
#define HWCFG_GATEWAY_OFFSET     36U
#define HWCFG_MASK_OFFSET        40U
#define HWCFG_DHCP_OFFSET        44U
#define HWCFG_WEB_PORT_OFFSET    45U
#define HWCFG_UPDATE_FLAG_OFFSET 64U
#define PORTCFG0_OFFSET          74U
#define PORTCFG1_OFFSET          139U
#define PORTCFG_BAUD_OFFSET      12U
#define PORTCFG_DATABITS_OFFSET  16U
#define PORTCFG_STOPBITS_OFFSET  17U
#define PORTCFG_PARITY_OFFSET    18U

static const uint8_t ch9121_flag[16] =
{
  'C', 'H', '9', '1', '2', '1', '_', 'C', 'F', 'G', '_', 'F', 'L', 'A', 'G', 0U
};

typedef struct
{
  uint8_t name[CH9121_NAME_LENGTH];
  uint8_t dhcp;
  uint8_t ip[4];
  uint8_t mask[4];
  uint8_t gateway[4];
  ch9121_port_config_t port1;
} ch9121_config_t;

volatile ch9121_diagnostics_t g_ch9121_diagnostics =
{
  CH9121_STATUS_OK, 0UL, 0UL, 0UL, 0UL, 0UL,
  CH9121_INIT_IDLE, ERR_INPROGRESS, ERR_INPROGRESS, 0U
};

static struct netif *service_netif;
static struct udp_pcb *service_pcb;
static ch9121_config_t active_config;
static ch9121_config_t pending_config;
static uint8_t apply_pending;

static void set_ipv4(ip4_addr_t *address, const uint8_t bytes[4])
{
  IP4_ADDR(address, bytes[0], bytes[1], bytes[2], bytes[3]);
}

static void get_ipv4_bytes(const ip4_addr_t *address, uint8_t bytes[4])
{
  bytes[0] = ip4_addr1(address);
  bytes[1] = ip4_addr2(address);
  bytes[2] = ip4_addr3(address);
  bytes[3] = ip4_addr4(address);
}

static void configure_netif(const ch9121_config_t *config)
{
  ip4_addr_t ip;
  ip4_addr_t mask;
  ip4_addr_t gateway;

  set_ipv4(&ip, config->ip);
  set_ipv4(&mask, config->mask);
  set_ipv4(&gateway, config->gateway);
  netif_set_addr(service_netif, &ip, &mask, &gateway);
}

static uint32_t ipv4_u32(const uint8_t address[4])
{
  return ((uint32_t)address[0] << 24U) |
         ((uint32_t)address[1] << 16U) |
         ((uint32_t)address[2] << 8U) |
         (uint32_t)address[3];
}

static uint8_t valid_unicast_host(uint32_t address)
{
  uint8_t first = (uint8_t)(address >> 24U);

  return (address != 0U) && (address != 0xFFFFFFFFUL) &&
         (first != 0U) && (first < 224U) && (first != 127U);
}

static uint8_t valid_static_config(const ch9121_config_t *config)
{
  uint32_t ip;
  uint32_t mask;
  uint32_t gateway;
  uint32_t inverse_mask;
  uint32_t host;

  ip = ipv4_u32(config->ip);
  mask = ipv4_u32(config->mask);
  gateway = ipv4_u32(config->gateway);

  if ((mask == 0U) || (mask == 0xFFFFFFFFUL) || !valid_unicast_host(ip))
  {
    return 0U;
  }

  inverse_mask = ~mask;
  if ((inverse_mask & (inverse_mask + 1U)) != 0U)
  {
    return 0U;
  }

  host = ip & inverse_mask;
  if ((host == 0U) || (host == inverse_mask))
  {
    return 0U;
  }

  if (gateway != 0U)
  {
    host = gateway & inverse_mask;
    if (!valid_unicast_host(gateway) || (gateway == ip) || ((gateway & mask) != (ip & mask)) ||
        (host == 0U) || (host == inverse_mask))
    {
      return 0U;
    }
  }

  return 1U;
}

static uint8_t parse_name(const uint8_t field[CH9121_NAME_LENGTH], uint8_t output[CH9121_NAME_LENGTH])
{
  uint32_t i;
  uint32_t length = 0U;
  uint8_t terminated = 0U;

  for (i = 0U; i < CH9121_NAME_LENGTH; ++i)
  {
    uint8_t character = field[i];

    if (terminated != 0U)
    {
      if (character != 0U)
      {
        return 0U;
      }
    }
    else if (character == 0U)
    {
      terminated = 1U;
    }
    else
    {
      if ((character < 0x20U) || (character > 0x7EU))
      {
        return 0U;
      }
      ++length;
    }
  }

  if ((terminated == 0U) || (length == 0U) || (length > 20U))
  {
    return 0U;
  }

  memcpy(output, field, CH9121_NAME_LENGTH);
  return 1U;
}

static uint8_t byte_is_supported_change(uint32_t offset)
{
  return ((offset >= HWCFG_NAME_OFFSET) && (offset < HWCFG_NAME_OFFSET + CH9121_NAME_LENGTH)) ||
         ((offset >= HWCFG_IP_OFFSET) && (offset < HWCFG_DHCP_OFFSET + 1U)) ||
         ((offset >= PORTCFG1_OFFSET) &&
          ch9121_port_mutable_byte(offset - PORTCFG1_OFFSET)) ||
         (offset == PORTCFG0_OFFSET + CH9121_PORT_NAGLE_OFFSET);
}

static void encode_config(const ch9121_config_t *config, uint8_t output[CH9121_DATA_SIZE])
{
  uint32_t port;
  uint32_t i;

  memset(output, 0, CH9121_DATA_SIZE);
  output[0] = 0x21U;
  output[1] = 0x21U;
  output[2] = 0x01U;
  output[3] = 0x04U;
  output[4] = 0x07U;
  memcpy(&output[HWCFG_NAME_OFFSET], config->name, CH9121_NAME_LENGTH);
  memcpy(&output[HWCFG_MAC_OFFSET], service_netif->hwaddr, ETH_HWADDR_LEN);
  memcpy(&output[HWCFG_IP_OFFSET], config->ip, sizeof(config->ip));
  memcpy(&output[HWCFG_GATEWAY_OFFSET], config->gateway, sizeof(config->gateway));
  memcpy(&output[HWCFG_MASK_OFFSET], config->mask, sizeof(config->mask));
  output[HWCFG_DHCP_OFFSET] = config->dhcp;
  output[HWCFG_WEB_PORT_OFFSET] = 0x50U; /* Read-only CH9121 sample value: port 80, little-endian. */
  output[HWCFG_UPDATE_FLAG_OFFSET] = 0xFFU; /* Read-only CH9121 sample value. */

  if (config->dhcp != 0U)
  {
    if (dhcp_supplied_address(service_netif))
    {
      get_ipv4_bytes(netif_ip4_addr(service_netif), &output[HWCFG_IP_OFFSET]);
      get_ipv4_bytes(netif_ip4_netmask(service_netif), &output[HWCFG_MASK_OFFSET]);
      get_ipv4_bytes(netif_ip4_gw(service_netif), &output[HWCFG_GATEWAY_OFFSET]);
    }
    else
    {
      memset(&output[HWCFG_IP_OFFSET], 0, 12U);
    }
  }

  for (port = 0U; port < 2U; ++port)
  {
    uint32_t base = (port == 0U) ? PORTCFG0_OFFSET : PORTCFG1_OFFSET;
    output[base] = (uint8_t)port;
    output[base + 1U] = 0U; /* Port 2 remains disabled. */
    output[base + PORTCFG_BAUD_OFFSET] = 0x00U;
    output[base + PORTCFG_BAUD_OFFSET + 1U] = 0x4BU;
    output[base + PORTCFG_BAUD_OFFSET + 2U] = 0x00U;
    output[base + PORTCFG_BAUD_OFFSET + 3U] = 0x00U;
    output[base + PORTCFG_DATABITS_OFFSET] = 0x08U;
    output[base + PORTCFG_STOPBITS_OFFSET] = 0x01U;
    output[base + PORTCFG_PARITY_OFFSET] = 0x04U;
    output[base + CH9121_PORT_NAGLE_OFFSET] = config->port1.nagle;
  }

  ch9121_port_encode(&config->port1, &output[PORTCFG1_OFFSET]);

  for (i = CH9121_CONFIG_SIZE; i < CH9121_DATA_SIZE; ++i)
  {
    output[i] = 0U;
  }
}

static void make_reply(uint8_t packet[CH9121_PACKET_SIZE], uint8_t command,
                       const uint8_t pc_mac[6], uint8_t length)
{
  memset(packet, 0, CH9121_PACKET_SIZE);
  memcpy(packet, ch9121_flag, sizeof(ch9121_flag));
  packet[16] = command;
  memcpy(&packet[17], service_netif->hwaddr, ETH_HWADDR_LEN);
  if (pc_mac != NULL)
  {
    memcpy(&packet[23], pc_mac, 6U);
  }
  packet[29] = length;
}

static err_t send_reply(const uint8_t packet[CH9121_PACKET_SIZE])
{
  struct pbuf *p;
  ip_addr_t broadcast;
  err_t result;

  p = pbuf_alloc(PBUF_TRANSPORT, CH9121_PACKET_SIZE, PBUF_RAM);
  if (p == NULL)
  {
    ++g_ch9121_diagnostics.tx_errors;
    g_ch9121_diagnostics.last_status = CH9121_STATUS_SEND_ERROR;
    g_ch9121_diagnostics.last_tx_result = ERR_MEM;
    return ERR_MEM;
  }

  if (pbuf_take(p, packet, CH9121_PACKET_SIZE) != ERR_OK)
  {
    pbuf_free(p);
    ++g_ch9121_diagnostics.tx_errors;
    g_ch9121_diagnostics.last_status = CH9121_STATUS_SEND_ERROR;
    g_ch9121_diagnostics.last_tx_result = ERR_MEM;
    return ERR_MEM;
  }

  IP_ADDR4(&broadcast, 255, 255, 255, 255);
  /* RAW UDP ports are host-order values; lwIP writes the wire-order header. */
  result = udp_sendto_if(service_pcb, p, &broadcast, CH9121_CLIENT_PORT, service_netif);
  pbuf_free(p);
  g_ch9121_diagnostics.last_tx_result = result;
  if (result == ERR_OK)
  {
    ++g_ch9121_diagnostics.tx_packets;
    g_ch9121_diagnostics.last_tx_ms = sys_now();
    g_ch9121_diagnostics.last_status = CH9121_STATUS_OK;
  }
  else
  {
    ++g_ch9121_diagnostics.tx_errors;
    g_ch9121_diagnostics.last_status = CH9121_STATUS_SEND_ERROR;
  }
  return result;
}

static uint8_t decode_candidate(const uint8_t data[CH9121_DATA_SIZE], ch9121_config_t *candidate)
{
  uint8_t expected[CH9121_DATA_SIZE];
  uint32_t i;

  *candidate = active_config;
  encode_config(&active_config, expected);
  for (i = 0U; i < CH9121_DATA_SIZE; ++i)
  {
    if ((byte_is_supported_change(i) == 0U) && (data[i] != expected[i]))
    {
      g_ch9121_diagnostics.last_status = CH9121_STATUS_READ_ONLY_CHANGED;
      return 0U;
    }
  }

  if (!parse_name(&data[HWCFG_NAME_OFFSET], candidate->name) ||
      ((data[HWCFG_DHCP_OFFSET] != 0U) && (data[HWCFG_DHCP_OFFSET] != 1U)))
  {
    g_ch9121_diagnostics.last_status = CH9121_STATUS_BAD_CONFIG;
    return 0U;
  }

  candidate->dhcp = data[HWCFG_DHCP_OFFSET];
  if (candidate->dhcp == 0U)
  {
    memcpy(candidate->ip, &data[HWCFG_IP_OFFSET], sizeof(candidate->ip));
    memcpy(candidate->gateway, &data[HWCFG_GATEWAY_OFFSET], sizeof(candidate->gateway));
    memcpy(candidate->mask, &data[HWCFG_MASK_OFFSET], sizeof(candidate->mask));
    if (!valid_static_config(candidate))
    {
      g_ch9121_diagnostics.last_status = CH9121_STATUS_BAD_CONFIG;
      return 0U;
    }
  }
  else
  {
    /* DHCP-controlled address fields are display-only; keep the static backup. */
    memcpy(candidate->ip, active_config.ip, sizeof(candidate->ip));
    memcpy(candidate->gateway, active_config.gateway, sizeof(candidate->gateway));
    memcpy(candidate->mask, active_config.mask, sizeof(candidate->mask));
  }

  if (!ch9121_port_decode(&data[PORTCFG1_OFFSET], &candidate->port1) ||
      (data[PORTCFG0_OFFSET + CH9121_PORT_NAGLE_OFFSET] > 1U) ||
      ((data[PORTCFG0_OFFSET + CH9121_PORT_NAGLE_OFFSET] != active_config.port1.nagle) &&
       (data[PORTCFG0_OFFSET + CH9121_PORT_NAGLE_OFFSET] != candidate->port1.nagle)))
  {
    g_ch9121_diagnostics.last_status = CH9121_STATUS_BAD_CONFIG;
    return 0U;
  }

  return 1U;
}

static void send_nak(const uint8_t request[CH9121_PACKET_SIZE])
{
  uint8_t response[CH9121_PACKET_SIZE];
  ch9121_status_t reason = g_ch9121_diagnostics.last_status;

  g_ch9121_diagnostics.last_rejection = reason;
  make_reply(response, CH9121_NAK_SET, &request[23], 0U);
  memcpy(&response[CH9121_HEADER_SIZE], &request[CH9121_HEADER_SIZE], CH9121_DATA_SIZE);
  if (send_reply(response) == ERR_OK)
  {
    g_ch9121_diagnostics.last_status = reason;
  }
}

static void receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                             const ip_addr_t *address, u16_t port)
{
  uint8_t request[CH9121_PACKET_SIZE];
  uint8_t response[CH9121_PACKET_SIZE];
  ch9121_config_t candidate;
  uint8_t *data;
  uint8_t command;
  uint8_t sender[4];
  uint32_t i;
  err_t result;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  g_ch9121_diagnostics.last_sender_port = port;
  get_ipv4_bytes(ip_2_ip4(address), sender);
  for (i = 0U; i < 4U; ++i)
  {
    g_ch9121_diagnostics.last_sender_ip[i] = sender[i];
  }

  if (p == NULL)
  {
    return;
  }
  ++g_ch9121_diagnostics.rx_packets;
  g_ch9121_diagnostics.last_rx_ms = sys_now();

  if ((p->tot_len != CH9121_PACKET_SIZE) ||
      (pbuf_copy_partial(p, request, CH9121_PACKET_SIZE, 0U) != CH9121_PACKET_SIZE) ||
      (memcmp(request, ch9121_flag, sizeof(ch9121_flag)) != 0))
  {
    ++g_ch9121_diagnostics.rejected_packets;
    g_ch9121_diagnostics.last_status = CH9121_STATUS_BAD_PACKET;
    g_ch9121_diagnostics.last_rejection = CH9121_STATUS_BAD_PACKET;
    pbuf_free(p);
    return;
  }

  command = request[16];
  g_ch9121_diagnostics.last_command = command;
  data = &request[CH9121_HEADER_SIZE];
  if (command == CH9121_CMD_SEARCH)
  {
    uint8_t name_length = 0U;

    ++g_ch9121_diagnostics.search_requests;

    for (i = 0U; i < ETH_HWADDR_LEN; ++i)
    {
      if (request[17U + i] != 0U)
      {
        break;
      }
    }
    if ((i != ETH_HWADDR_LEN) || (request[29] != 0U))
    {
      ++g_ch9121_diagnostics.rejected_packets;
      g_ch9121_diagnostics.last_status = CH9121_STATUS_BAD_PACKET;
      g_ch9121_diagnostics.last_rejection = CH9121_STATUS_BAD_PACKET;
      pbuf_free(p);
      return;
    }

    while ((name_length < 20U) && (active_config.name[name_length] != 0U))
    {
      ++name_length;
    }
    make_reply(response, CH9121_ACK_SEARCH, NULL, (uint8_t)(5U + name_length));
    if (active_config.dhcp != 0U)
    {
      if (dhcp_supplied_address(service_netif))
      {
        get_ipv4_bytes(netif_ip4_addr(service_netif), &response[CH9121_HEADER_SIZE]);
      }
    }
    else
    {
      memcpy(&response[CH9121_HEADER_SIZE], active_config.ip, 4U);
    }
    memcpy(&response[CH9121_HEADER_SIZE + 4U], active_config.name, name_length);
    response[CH9121_HEADER_SIZE + 4U + name_length] = 0U;
    response[CH9121_HEADER_SIZE + 5U + name_length] = 0x2FU;
    result = send_reply(response);
    if (result != ERR_OK)
    {
      ++g_ch9121_diagnostics.rejected_packets;
    }
  }
  else if ((command == CH9121_CMD_GET) || (command == CH9121_CMD_SET))
  {
    if (command == CH9121_CMD_GET)
    {
      ++g_ch9121_diagnostics.get_requests;
    }
    else
    {
      ++g_ch9121_diagnostics.set_requests;
    }
    for (i = 0U; i < ETH_HWADDR_LEN; ++i)
    {
      if (request[17U + i] != service_netif->hwaddr[i])
      {
        break;
      }
    }
    if ((i != ETH_HWADDR_LEN) ||
        (request[29] != ((command == CH9121_CMD_GET) ? 0U : 0xCCU)))
    {
      ++g_ch9121_diagnostics.rejected_packets;
      g_ch9121_diagnostics.last_status = CH9121_STATUS_BAD_PACKET;
      g_ch9121_diagnostics.last_rejection = CH9121_STATUS_BAD_PACKET;
      pbuf_free(p);
      return;
    }

    if (command == CH9121_CMD_GET)
    {
      make_reply(response, CH9121_ACK_GET, &request[23], 0xCCU);
      encode_config(&active_config, &response[CH9121_HEADER_SIZE]);
      result = send_reply(response);
      if (result != ERR_OK)
      {
        ++g_ch9121_diagnostics.rejected_packets;
      }
    }
    else if (apply_pending != 0U)
    {
      g_ch9121_diagnostics.last_status = CH9121_STATUS_BUSY;
      send_nak(request);
    }
    else
    {
      if (!decode_candidate(data, &candidate))
      {
        ++g_ch9121_diagnostics.rejected_packets;
        send_nak(request);
      }
      else
      {
        make_reply(response, CH9121_ACK_SET, &request[23], 0U);
        memcpy(&response[CH9121_HEADER_SIZE], data, CH9121_DATA_SIZE);
        /* Port 1 owns the shared Nagle setting. Accept an unchanged port 2
         * shadow or a matching new shadow, and reply with the normalized value.
         */
        response[CH9121_HEADER_SIZE + PORTCFG0_OFFSET + CH9121_PORT_NAGLE_OFFSET] =
          candidate.port1.nagle;
        result = send_reply(response);
        if (result == ERR_OK)
        {
          pending_config = candidate;
          apply_pending = 1U;
          ++g_ch9121_diagnostics.accepted_sets;
          for (i = 0U; i < 4U; ++i)
          {
            g_ch9121_diagnostics.last_set_sender_ip[i] = g_ch9121_diagnostics.last_sender_ip[i];
          }
          for (i = 0U; i < 6U; ++i)
          {
            g_ch9121_diagnostics.last_set_pc_mac[i] = request[23U + i];
          }
        }
        else
        {
          ++g_ch9121_diagnostics.rejected_packets;
        }
      }
    }
  }

  pbuf_free(p);
}

err_t ch9121_service_init(struct netif *netif)
{
  ip4_addr_t ip;
  ip4_addr_t mask;
  ip4_addr_t gateway;
  err_t result;

  g_ch9121_diagnostics.init_stage = CH9121_INIT_CONFIG;
  g_ch9121_diagnostics.init_result = ERR_INPROGRESS;
  g_ch9121_diagnostics.dhcp_start_result = ERR_INPROGRESS;
  g_ch9121_diagnostics.local_port = 0U;

  if (netif == NULL)
  {
    g_ch9121_diagnostics.init_result = ERR_ARG;
    return ERR_ARG;
  }

  service_netif = netif;
  service_pcb = NULL;
  apply_pending = 0U;
  memset(&active_config, 0, sizeof(active_config));
  memcpy(active_config.name, "F407_CH9121", sizeof("F407_CH9121"));
  active_config.ip[0] = 192U;
  active_config.ip[1] = 168U;
  active_config.ip[2] = 1U;
  active_config.ip[3] = 52U;
  active_config.mask[0] = 255U;
  active_config.mask[1] = 255U;
  active_config.mask[2] = 255U;
  active_config.mask[3] = 0U;
  active_config.gateway[0] = 192U;
  active_config.gateway[1] = 168U;
  active_config.gateway[2] = 1U;
  active_config.gateway[3] = 1U;
  /* Keep a static backup for a later switch to static mode, but start this
     validation build with DHCP enabled and no preassigned IPv4 address. */
  active_config.dhcp = 1U;
  ch9121_port_defaults(&active_config.port1);

  IP4_ADDR(&ip, 0U, 0U, 0U, 0U);
  IP4_ADDR(&mask, 0U, 0U, 0U, 0U);
  IP4_ADDR(&gateway, 0U, 0U, 0U, 0U);
  netif_set_addr(service_netif, &ip, &mask, &gateway);
  if (!netif_is_up(service_netif))
  {
    netif_set_up(service_netif);
  }

  g_ch9121_diagnostics.init_stage = CH9121_INIT_UDP_ALLOC;
  service_pcb = udp_new_ip_type(IPADDR_TYPE_V4);
  if (service_pcb == NULL)
  {
    g_ch9121_diagnostics.last_status = CH9121_STATUS_BIND_ERROR;
    g_ch9121_diagnostics.init_result = ERR_MEM;
    return ERR_MEM;
  }

  g_ch9121_diagnostics.init_stage = CH9121_INIT_UDP_BIND;
  result = udp_bind(service_pcb, IP_ADDR_ANY, CH9121_DEVICE_PORT);
  if (result != ERR_OK)
  {
    udp_remove(service_pcb);
    service_pcb = NULL;
    g_ch9121_diagnostics.last_status = CH9121_STATUS_BIND_ERROR;
    g_ch9121_diagnostics.init_result = result;
    return result;
  }

  g_ch9121_diagnostics.local_port = service_pcb->local_port;
  udp_recv(service_pcb, receive_callback, NULL);
  g_ch9121_diagnostics.init_stage = CH9121_INIT_DHCP_START;
  result = dhcp_start(service_netif);
  g_ch9121_diagnostics.dhcp_start_result = result;
  if (result != ERR_OK)
  {
    udp_remove(service_pcb);
    service_pcb = NULL;
    ++g_ch9121_diagnostics.apply_errors;
    g_ch9121_diagnostics.last_status = CH9121_STATUS_APPLY_ERROR;
    g_ch9121_diagnostics.init_result = result;
    return result;
  }
  g_ch9121_diagnostics.last_status = CH9121_STATUS_OK;
  g_ch9121_diagnostics.init_result = ERR_OK;
  ch9121_transport_init(service_netif, &active_config.port1);
  g_ch9121_diagnostics.init_stage = CH9121_INIT_READY;
  return ERR_OK;
}

void ch9121_service_process(void)
{
  ch9121_config_t previous;
  err_t result;

  ch9121_transport_process();

  if ((apply_pending == 0U) || (service_netif == NULL))
  {
    return;
  }

  previous = active_config;
  if (!netif_is_up(service_netif))
  {
    netif_set_up(service_netif);
  }
  if ((previous.dhcp == 0U) && (pending_config.dhcp != 0U))
  {
    result = dhcp_start(service_netif);
    if (result != ERR_OK)
    {
      configure_netif(&previous);
      ++g_ch9121_diagnostics.apply_errors;
      g_ch9121_diagnostics.last_status = CH9121_STATUS_APPLY_ERROR;
      apply_pending = 0U;
      return;
    }
  }
  else if ((previous.dhcp != 0U) && (pending_config.dhcp == 0U))
  {
    dhcp_stop(service_netif);
    configure_netif(&pending_config);
  }
  else if (pending_config.dhcp == 0U)
  {
    configure_netif(&pending_config);
  }

  active_config = pending_config;
  if (!ch9121_port_equal(&previous.port1, &active_config.port1))
  {
    ch9121_transport_configure(&active_config.port1);
  }
  ++g_ch9121_diagnostics.applied_sets;
  apply_pending = 0U;
  g_ch9121_diagnostics.last_status = CH9121_STATUS_OK;
}
