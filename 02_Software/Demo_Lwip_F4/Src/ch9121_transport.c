#include "ch9121_transport.h"
#include "ch9121_protocol.h"

#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"

#include <string.h>

#define CH9121_RETRY_MS         1000U
#define CH9121_DNS_REFRESH_MS   60000U

volatile ch9121_transport_diagnostics_t g_ch9121_transport_diagnostics;

static struct netif *transport_netif;
static ch9121_port_config_t port_config;
static struct tcp_pcb *listener;
static struct tcp_pcb *connection;
static struct udp_pcb *udp_socket;
static ip_addr_t destination;
static ip4_addr_t previous_ip;
static uint8_t previous_link;
static uint8_t restart_pending;
static uint8_t connected;
static uint8_t peer_closed;
static uint8_t dns_pending;
static uint8_t destination_ready;
static uint32_t generation;
static uint32_t retry_at;
static uint32_t dns_refresh_at;

static err_t tcp_received(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t error);
static err_t tcp_sent_data(void *arg, struct tcp_pcb *pcb, u16_t length);
static err_t tcp_polled(void *arg, struct tcp_pcb *pcb);
static void tcp_failed(void *arg, err_t error);

static void record_error(err_t error)
{
  g_ch9121_transport_diagnostics.last_error = error;
  ++g_ch9121_transport_diagnostics.errors;
  retry_at = sys_now() + CH9121_RETRY_MS;
}

static void set_peer(const ip_addr_t *address)
{
  g_ch9121_transport_diagnostics.peer_ip[0] = ip4_addr1(ip_2_ip4(address));
  g_ch9121_transport_diagnostics.peer_ip[1] = ip4_addr2(ip_2_ip4(address));
  g_ch9121_transport_diagnostics.peer_ip[2] = ip4_addr3(ip_2_ip4(address));
  g_ch9121_transport_diagnostics.peer_ip[3] = ip4_addr4(ip_2_ip4(address));
}

static void attach_callbacks(struct tcp_pcb *pcb)
{
  /* The PCB itself identifies a session; old error callbacks cannot clear
   * a newer connection after a configuration change or graceful close.
   */
  tcp_arg(pcb, pcb);
  tcp_recv(pcb, tcp_received);
  tcp_sent(pcb, tcp_sent_data);
  tcp_poll(pcb, tcp_polled, 2U);
  tcp_err(pcb, tcp_failed);
}

static void detach_callbacks(struct tcp_pcb *pcb)
{
  tcp_arg(pcb, NULL);
  tcp_recv(pcb, NULL);
  tcp_sent(pcb, NULL);
  tcp_poll(pcb, NULL, 0U);
  tcp_err(pcb, NULL);
}

static void stop_sockets(void)
{
  ++generation;
  dns_pending = 0U;
  destination_ready = 0U;
  if (connection != NULL)
  {
    struct tcp_pcb *pcb = connection;
    connection = NULL;
    detach_callbacks(pcb);
    tcp_abort(pcb);
  }
  if (listener != NULL)
  {
    tcp_accept(listener, NULL);
    tcp_arg(listener, NULL);
    (void)tcp_close(listener); /* LISTEN has no queued data: cannot return ERR_MEM. */
    listener = NULL;
  }
  if (udp_socket != NULL)
  {
    udp_remove(udp_socket);
    udp_socket = NULL;
  }
  connected = 0U;
  peer_closed = 0U;
  ch9121_protocol_reset_session();
  g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_DOWN;
  g_ch9121_transport_diagnostics.local_port = 0U;
}

static void finish_close(void)
{
  struct tcp_pcb *pcb = connection;
  err_t result;

  if ((pcb == NULL) || (ch9121_protocol_tx_pending() != 0U))
  {
    return;
  }
  detach_callbacks(pcb);
  result = tcp_close(pcb);
  if (result == ERR_OK)
  {
    connection = NULL;
    connected = 0U;
    peer_closed = 0U;
    ch9121_protocol_reset_session();
    retry_at = sys_now() + CH9121_RETRY_MS;
    g_ch9121_transport_diagnostics.state = (listener != NULL) ?
      CH9121_TRANSPORT_LISTEN : CH9121_TRANSPORT_DOWN;
  }
  else
  {
    attach_callbacks(pcb); /* Retry close later if a FIN cannot be allocated. */
  }
}

static err_t tcp_received(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t error)
{
  uint8_t chunk[256];
  uint16_t offset = 0U;
  uint16_t length;
  uint16_t copied;

  LWIP_UNUSED_ARG(arg);
  if (p == NULL)
  {
    peer_closed = 1U;
    return ERR_OK;
  }
  if (error != ERR_OK)
  {
    return error; /* lwIP retains ownership when the callback refuses data. */
  }
  if (p->tot_len > ch9121_protocol_rx_available())
  {
    return ERR_MEM;
  }
  while (offset < p->tot_len)
  {
    length = (uint16_t)(p->tot_len - offset);
    if (length > sizeof(chunk))
      length = sizeof(chunk);
    copied = pbuf_copy_partial(p, chunk, length, offset);
    if ((copied != length) || !ch9121_protocol_receive(chunk, length))
    {
      ch9121_protocol_reset_session();
      return ERR_MEM;
    }
    offset = (uint16_t)(offset + length);
  }
  g_ch9121_transport_diagnostics.rx_bytes += p->tot_len;
  tcp_recved(pcb, p->tot_len);
  pbuf_free(p);
  return ERR_OK;
}

static err_t tcp_sent_data(void *arg, struct tcp_pcb *pcb, u16_t length)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(length);
  return ERR_OK;
}

static err_t tcp_polled(void *arg, struct tcp_pcb *pcb)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  return ERR_OK;
}

static void tcp_failed(void *arg, err_t error)
{
  /* The PCB has already been freed by lwIP; only compare its saved address. */
  if (arg == connection)
  {
    connection = NULL;
    connected = 0U;
    peer_closed = 0U;
    ch9121_protocol_reset_session();
    record_error(error);
    g_ch9121_transport_diagnostics.state = (listener != NULL) ?
      CH9121_TRANSPORT_LISTEN : CH9121_TRANSPORT_DOWN;
  }
}

static void session_ready(struct tcp_pcb *pcb)
{
  connected = 1U;
  peer_closed = 0U;
  ch9121_protocol_reset_session();
  if (port_config.nagle != 0U)
  {
    tcp_nagle_enable(pcb);
  }
  else
  {
    tcp_nagle_disable(pcb);
  }
  set_peer(&pcb->remote_ip);
  ++g_ch9121_transport_diagnostics.connections;
  g_ch9121_transport_diagnostics.local_port = pcb->local_port;
  g_ch9121_transport_diagnostics.last_error = ERR_OK;
  g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_CONNECTED;
}

static err_t ch9121_tcp_accept_callback(void *arg, struct tcp_pcb *pcb, err_t error)
{
  LWIP_UNUSED_ARG(arg);
  if ((error != ERR_OK) || (pcb == NULL))
  {
    return error;
  }
  if (connection != NULL)
  {
    tcp_abort(pcb); /* One transparent-channel peer at a time. */
    return ERR_ABRT;
  }
  connection = pcb;
  attach_callbacks(pcb);
  session_ready(pcb);
  return ERR_OK;
}

static err_t tcp_connected(void *arg, struct tcp_pcb *pcb, err_t error)
{
  LWIP_UNUSED_ARG(arg);
  if (error == ERR_OK)
  {
    session_ready(pcb);
  }
  return error;
}

static void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                         const ip_addr_t *address, u16_t port)
{
  err_t result;
  u16_t length;

  LWIP_UNUSED_ARG(arg);
  if (p == NULL)
  {
    return;
  }
  if ((port_config.mode == CH9121_MODE_UDP_CLIENT) &&
      (port != port_config.destination_port))
  {
    pbuf_free(p);
    return;
  }
  length = p->tot_len;
  g_ch9121_transport_diagnostics.rx_bytes += length;
  set_peer(address);
  /* Preserve UDP datagram boundaries in the echo backend. */
  result = udp_sendto_if(pcb, p, address, port, transport_netif);
  if (result == ERR_OK)
  {
    g_ch9121_transport_diagnostics.tx_bytes += length;
  }
  else
  {
    record_error(result);
  }
  pbuf_free(p);
}

static void dns_resolved(const char *name, const ip_addr_t *address, void *arg)
{
  LWIP_UNUSED_ARG(name);
  if ((uint32_t)(uintptr_t)arg != generation)
  {
    return; /* Ignore results for a superseded configuration. */
  }
  dns_pending = 0U;
  if ((address != NULL) && IP_IS_V4(address) && !ip_addr_isany(address))
  {
    ip_addr_copy(destination, *address);
    destination_ready = 1U;
  }
  else
  {
    record_error(ERR_VAL);
  }
}

static uint8_t resolve_destination(void)
{
  err_t result;

  if (destination_ready != 0U)
  {
    destination_ready = 0U; /* Consume an asynchronous DNS result once. */
    return 1U;
  }
  if (port_config.use_domain == 0U)
  {
    IP_ADDR4(&destination, port_config.destination_ip[0], port_config.destination_ip[1],
             port_config.destination_ip[2], port_config.destination_ip[3]);
    return 1U;
  }
  if (dns_pending != 0U)
  {
    return 0U;
  }
  /* DHCP supplies DNS when available. Static mode uses its gateway as a
   * resolver, replacing any resolver left over from a previous DHCP lease.
   */
  if (!dhcp_supplied_address(transport_netif) || ip_addr_isany(dns_getserver(0U)))
  {
    ip_addr_t gateway;
    ip_addr_copy_from_ip4(gateway, *netif_ip4_gw(transport_netif));
    dns_setserver(0U, &gateway);
  }
  result = dns_gethostbyname((const char *)port_config.domain, &destination,
                            dns_resolved, (void *)(uintptr_t)generation);
  if (result == ERR_OK)
  {
    return 1U;
  }
  if (result == ERR_INPROGRESS)
  {
    dns_pending = 1U;
    g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_RESOLVING;
  }
  else
  {
    record_error(result);
  }
  return 0U;
}

static err_t set_udp_destination(void)
{
  if (ip4_addr_isbroadcast(ip_2_ip4(&destination), transport_netif))
  {
    /* Replies to a broadcast have unicast source addresses. A connected UDP
     * PCB would discard them because its remote address is the broadcast IP.
     * The receive callback still restricts peers to the configured source port.
     */
    udp_disconnect(udp_socket);
    return ERR_OK;
  }
  return udp_connect(udp_socket, &destination, port_config.destination_port);
}

static void open_socket(void)
{
  struct tcp_pcb *pcb;
  struct tcp_pcb *listening;
  err_t result;

  retry_at = sys_now() + CH9121_RETRY_MS;
  ++g_ch9121_transport_diagnostics.retries;
  if (port_config.mode == CH9121_MODE_TCP_SERVER)
  {
    pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (pcb == NULL)
    {
      record_error(ERR_MEM);
      return;
    }
    tcp_bind_netif(pcb, transport_netif);
    result = tcp_bind(pcb, IP_ADDR_ANY, port_config.local_port);
    if (result != ERR_OK)
    {
      (void)tcp_close(pcb);
      record_error(result);
      return;
    }
    listening = tcp_listen_with_backlog_and_err(pcb, 1U, &result);
    if (listening == NULL)
    {
      (void)tcp_close(pcb); /* The original PCB survives a failed listen. */
      record_error(result);
      return;
    }
    listener = listening;
    tcp_accept(listener, ch9121_tcp_accept_callback);
    g_ch9121_transport_diagnostics.local_port = listener->local_port;
    g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_LISTEN;
  }
  else if (port_config.mode == CH9121_MODE_TCP_CLIENT)
  {
    if (!resolve_destination())
    {
      return;
    }
    pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (pcb == NULL)
    {
      record_error(ERR_MEM);
      return;
    }
    tcp_bind_netif(pcb, transport_netif);
    result = tcp_bind(pcb, IP_ADDR_ANY, (port_config.random_port != 0U) ? 0U : port_config.local_port);
    if (result != ERR_OK)
    {
      (void)tcp_close(pcb);
      record_error(result);
      return;
    }
    connection = pcb;
    attach_callbacks(pcb);
    result = tcp_connect(pcb, &destination, port_config.destination_port, tcp_connected);
    if (result != ERR_OK)
    {
      connection = NULL;
      detach_callbacks(pcb);
      tcp_abort(pcb);
      record_error(result);
      return;
    }
    g_ch9121_transport_diagnostics.local_port = pcb->local_port;
    g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_CONNECTING;
  }
  else
  {
    if ((port_config.mode == CH9121_MODE_UDP_CLIENT) && !resolve_destination())
    {
      return;
    }
    udp_socket = udp_new_ip_type(IPADDR_TYPE_V4);
    if (udp_socket == NULL)
    {
      record_error(ERR_MEM);
      return;
    }
    udp_bind_netif(udp_socket, transport_netif);
    result = udp_bind(udp_socket, IP_ADDR_ANY, port_config.local_port);
    if ((result == ERR_OK) && (port_config.mode == CH9121_MODE_UDP_CLIENT))
    {
      result = set_udp_destination();
    }
    if (result != ERR_OK)
    {
      udp_remove(udp_socket);
      udp_socket = NULL;
      record_error(result);
      return;
    }
    udp_recv(udp_socket, udp_received, NULL);
    g_ch9121_transport_diagnostics.local_port = udp_socket->local_port;
    g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_UDP;
    dns_refresh_at = sys_now() + CH9121_DNS_REFRESH_MS;
  }
  g_ch9121_transport_diagnostics.last_error = ERR_OK;
}

void ch9121_transport_init(struct netif *netif, const ch9121_port_config_t *config)
{
  transport_netif = netif;
  memset(&previous_ip, 0, sizeof(previous_ip));
  previous_link = 0U;
  ch9121_protocol_init();
  ch9121_transport_configure(config);
}

void ch9121_transport_configure(const ch9121_port_config_t *config)
{
  port_config = *config;
  restart_pending = 1U; /* Apply outside RAW callbacks, after the SET ACK. */
}

void ch9121_transport_process(void)
{
  uint8_t link;
  uint32_t now;

  if (transport_netif == NULL)
  {
    return;
  }
  now = sys_now();
  link = (uint8_t)(netif_is_up(transport_netif) && netif_is_link_up(transport_netif));
  if ((restart_pending != 0U) ||
      !ip4_addr_cmp(&previous_ip, netif_ip4_addr(transport_netif)) ||
      ((previous_link != 0U) && (link == 0U) && (port_config.close_on_link_loss != 0U)))
  {
    stop_sockets();
    restart_pending = 0U;
    ip4_addr_copy(previous_ip, *netif_ip4_addr(transport_netif));
    retry_at = now;
  }
  previous_link = link;
  if ((link == 0U) || ip4_addr_isany_val(*netif_ip4_addr(transport_netif)))
  {
    return;
  }
  if ((listener == NULL) && (connection == NULL) && (udp_socket == NULL) &&
      (dns_pending == 0U) && ((int32_t)(now - retry_at) >= 0))
  {
    open_socket();
  }
  if ((udp_socket != NULL) && (port_config.mode == CH9121_MODE_UDP_CLIENT) &&
      (port_config.use_domain != 0U) && ((int32_t)(now - dns_refresh_at) >= 0))
  {
    dns_refresh_at = now + CH9121_RETRY_MS;
    if (resolve_destination())
    {
      err_t result = set_udp_destination();
      if (result == ERR_OK)
      {
        dns_refresh_at = now + CH9121_DNS_REFRESH_MS;
        g_ch9121_transport_diagnostics.state = CH9121_TRANSPORT_UDP;
        g_ch9121_transport_diagnostics.last_error = ERR_OK;
      }
      else
      {
        record_error(result);
      }
    }
  }
  if ((connection != NULL) && (connected != 0U))
  {
    ch9121_protocol_process(connection);
    if (ch9121_protocol_failed())
    {
      struct tcp_pcb *pcb = connection;
      connection = NULL;
      connected = 0U;
      peer_closed = 0U;
      detach_callbacks(pcb);
      tcp_abort(pcb);
      ch9121_protocol_reset_session();
      g_ch9121_transport_diagnostics.state = (listener != NULL) ?
        CH9121_TRANSPORT_LISTEN : CH9121_TRANSPORT_DOWN;
    }
    else if ((peer_closed != 0U) && (ch9121_protocol_tx_pending() == 0U))
    {
      finish_close();
    }
  }
}
