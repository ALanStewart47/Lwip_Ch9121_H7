#include "ch9121_protocol.h"
#include "ch9121_transport.h"
#include "protocol_public.h"
#include "protocol_ram.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"

#include <string.h>

#define PROTOCOL_RX_CAPACITY       4096U
#define PROTOCOL_TX_CAPACITY       4096U
#define PROTOCOL_FRAME_MAX         SUM_SIZE
#define PROTOCOL_PARTIAL_TIMEOUT   250U

typedef enum
{
  FRAME_NEED_MORE = 0,
  FRAME_READY,
  FRAME_DISCARD,
  FRAME_FATAL
} frame_status_t;

static uint8_t rx_buffer[PROTOCOL_RX_CAPACITY];
static uint16_t rx_head;
static uint16_t rx_count;
static uint8_t tx_buffer[PROTOCOL_TX_CAPACITY];
static uint16_t tx_head;
static uint16_t tx_count;
static uint8_t frame_buffer[PROTOCOL_FRAME_MAX];
static uint8_t wait_active;
static uint32_t wait_started;
static uint8_t fatal_error;

static uint8_t is_digit(uint8_t value)
{
  return (uint8_t)((value >= '0') && (value <= '9'));
}

static uint8_t is_channel_letter(uint8_t value)
{
  return (uint8_t)((value >= 'A') && (value < ('A' + CHANNEL_NUM)));
}

static uint8_t xor_bytes(const uint8_t *data, uint16_t length)
{
  uint16_t i;
  uint8_t result = 0U;
  for (i = 0U; i < length; ++i)
    result ^= data[i];
  return result;
}

static void rx_copy(uint16_t offset, uint8_t *destination, uint16_t length)
{
  uint16_t i;
  for (i = 0U; i < length; ++i)
    destination[i] = rx_buffer[(rx_head + offset + i) % PROTOCOL_RX_CAPACITY];
}

static void rx_drop(uint16_t length)
{
  if (length > rx_count)
    length = rx_count;
  rx_head = (uint16_t)((rx_head + length) % PROTOCOL_RX_CAPACITY);
  rx_count = (uint16_t)(rx_count - length);
  wait_active = 0U;
}

static uint16_t tx_free(void)
{
  return (uint16_t)(PROTOCOL_TX_CAPACITY - tx_count);
}

static uint8_t tx_push(const uint8_t *data, uint16_t length)
{
  uint16_t tail;
  uint16_t first;
  if (length > tx_free())
    return 0U;
  tail = (uint16_t)((tx_head + tx_count) % PROTOCOL_TX_CAPACITY);
  first = (uint16_t)(PROTOCOL_TX_CAPACITY - tail);
  if (first > length)
    first = length;
  memcpy(&tx_buffer[tail], data, first);
  if (first < length)
    memcpy(tx_buffer, &data[first], (uint16_t)(length - first));
  tx_count = (uint16_t)(tx_count + length);
  return 1U;
}

static void reply_error(uint8_t head, uint8_t command, uint8_t detail, uint8_t code)
{
  uint8_t reply[4];
  reply[0] = head;
  reply[1] = command;
  reply[2] = detail;
  reply[3] = code;
  (void)tx_push(reply, sizeof(reply));
}

static uint8_t valid_ascii_s(const uint8_t *data, uint16_t length)
{
  uint16_t i;
  if ((length == 2U) &&
      ((data[1] == 'T') || (data[1] == 'W') || is_channel_letter(data[1])))
    return 1U;
  if ((length == 3U) && (data[2] == '#') &&
      ((data[1] == 'T') || (data[1] == 'W') || is_channel_letter(data[1])))
    return 1U;
  if ((length == 4U) && (data[1] == 'P') && is_channel_letter(data[2]) &&
      (data[3] == '#'))
    return 1U;
  if ((length == 5U) && (memcmp(data, "SAVE#", 5U) == 0))
    return 1U;
  if ((length == 5U) && (data[1] == 'P') && (data[2] == 'U') &&
      is_channel_letter(data[3]) && (data[4] == '#'))
    return 1U;
  if ((length == 5U) && (data[1] >= '1') && (data[1] <= '8') &&
      (data[4] == '#'))
    return 1U;
  if ((length == 6U) && (data[1] == 'P') && (data[2] == 'U') &&
      is_channel_letter(data[3]) && is_digit(data[4]) && (data[5] == '#'))
    return 1U;
  if (length == 7U)
  {
    if ((data[1] == 'W') && (data[2] == 'T') && (data[3] == 'R') &&
        (data[4] == 'I') && (data[5] == 'G') && (data[6] == '#'))
      return 1U;
    if ((data[1] == 'T') && (data[2] == '0') && (data[6] == '#'))
    {
      for (i = 3U; i <= 5U; ++i)
        if (!is_digit(data[i])) return 0U;
      return 1U;
    }
    if ((data[1] == 'W') && (data[6] == '#'))
    {
      for (i = 2U; i <= 5U; ++i)
        if (!is_digit(data[i])) return 0U;
      return 1U;
    }
    if (is_channel_letter(data[1]) && (data[2] == '0') && (data[6] == '#'))
    {
      for (i = 3U; i <= 5U; ++i)
        if (!is_digit(data[i])) return 0U;
      return 1U;
    }
  }
  if ((length == 8U) && (data[1] == 'P') && is_channel_letter(data[2]) &&
      (data[3] == '0') && (data[7] == '#'))
  {
    for (i = 4U; i <= 6U; ++i)
      if (!is_digit(data[i])) return 0U;
    return 1U;
  }
  return 0U;
}

static uint8_t valid_ascii_t(const uint8_t *data, uint16_t length)
{
  if ((length == 2U) && (data[1] == '#')) return 1U;
  if ((length == 3U) && (data[2] == '#') &&
      ((data[1] == 'H') || (data[1] == 'L') || (data[1] == 'P'))) return 1U;
  if ((length == 4U) && (data[1] == 'P') &&
      ((data[2] == '0') || (data[2] == '1')) && (data[3] == '#')) return 1U;
  return 0U;
}

static uint8_t valid_ascii_d(const uint8_t *data, uint16_t length)
{
  if ((length == 3U) && (data[1] == 'C') && (data[2] == '#')) return 1U;
  if ((length == 4U) && ((data[1] == 'L') || (data[1] == 'C')) &&
      is_channel_letter(data[2]) && (data[3] == '#')) return 1U;
  if ((length == 7U) && (data[1] == 'C') && (data[2] == '0') &&
      (data[6] == '#'))
  {
    return (uint8_t)(is_digit(data[3]) && is_digit(data[4]) && is_digit(data[5]));
  }
  if ((length == 8U) && ((data[1] == 'L') || (data[1] == 'C')) &&
      is_channel_letter(data[2]) && (data[3] == '0') && (data[7] == '#'))
  {
    return (uint8_t)(is_digit(data[4]) && is_digit(data[5]) && is_digit(data[6]));
  }
  return 0U;
}

static uint8_t valid_ascii_p(const uint8_t *data, uint16_t length)
{
  if ((length == 5U) && (memcmp(data, "PRCL#", 5U) == 0)) return 1U;
  if ((length == 5U) && (data[1] == 'R') && (data[2] == 'W') &&
      is_channel_letter(data[3]) && (data[4] == '#')) return 1U;
  if ((length == 5U) && (data[1] == 'R') && (data[2] == 'N') &&
      is_channel_letter(data[3]) && (data[4] == '#')) return 1U;
  if ((length == 7U) && (data[1] == 'R') && (data[2] == 'N') &&
      is_channel_letter(data[3]) && (data[6] == '#')) return 1U;
  if ((length == 9U) && (memcmp(data, "PRENCLR", 7U) == 0) &&
      is_channel_letter(data[7]) && (data[8] == '#')) return 1U;
  return 0U;
}

static uint8_t valid_ascii_extension(const uint8_t *data, uint16_t length)
{
  uint8_t function;
  uint16_t channels;
  uint16_t i;
  if ((length < 5U) || !is_digit(data[1]) || !is_digit(data[2]) ||
      (data[length - 1U] != '#')) return 0U;
  function = (uint8_t)((data[1] - '0') * 10U + (data[2] - '0'));
  if ((function < 1U) || (function > 13U)) return 0U;
  if (data[0] == '$')
  {
    if ((function == 4U) || (function == 8U) || (function == 9U))
    {
      uint16_t value;
      if ((length != 9U) || !is_channel_letter(data[3]) ||
          !is_digit(data[5]) || !is_digit(data[6]) || !is_digit(data[7]))
        return 0U;
      value = (uint16_t)((data[5] - '0') * 100U +
                         (data[6] - '0') * 10U + (data[7] - '0'));
      if ((function == 4U) && (value != 1U) && (value != 4U) && (value != 6U))
        return 0U;
      return 1U;
    }
    if ((length < 9U) || (((length - 4U) % 5U) != 0U)) return 0U;
    channels = (uint16_t)((length - 4U) / 5U);
    if ((channels == 0U) || (channels > CHANNEL_NUM)) return 0U;
    for (i = 0U; i < channels; ++i)
    {
      uint16_t base = (uint16_t)(3U + 5U * i);
      if (!is_channel_letter(data[base]) || !is_digit(data[base + 2U]) ||
          !is_digit(data[base + 3U]) || !is_digit(data[base + 4U])) return 0U;
    }
    return 1U;
  }
  if ((function == 4U) || (function == 8U) || (function == 9U))
    return (uint8_t)(length == 5U);
  channels = (uint16_t)(length - 4U);
  if ((channels == 0U) || (channels > CHANNEL_NUM)) return 0U;
  for (i = 0U; i < channels; ++i)
    if (!is_channel_letter(data[3U + i])) return 0U;
  return 1U;
}

static int find_terminator(const uint8_t *data, uint16_t length, uint16_t maximum)
{
  uint16_t i;
  uint16_t limit = (length < maximum) ? length : maximum;
  for (i = 0U; i < limit; ++i)
    if (data[i] == '#') return (int)(i + 1U);
  return (length >= maximum) ? -1 : 0;
}

static frame_status_t scan_frame(const uint8_t *data, uint16_t available,
                                 uint16_t *frame_length)
{
  uint16_t length;
  int terminator;
  uint16_t payload_length;
  uint16_t count;
  uint32_t wide_length;

  *frame_length = 0U;
  if (available == 0U) return FRAME_NEED_MORE;

  if (data[0] == 0xCAU)
  {
    if (available < 6U) return FRAME_NEED_MORE;
    *frame_length = 6U;
    return (xor_bytes(data, 5U) == data[5]) ? FRAME_READY : FRAME_DISCARD;
  }
  if (data[0] == 0xCBU)
  {
    if (available < 3U) return FRAME_NEED_MORE;
    count = data[2];
    length = (uint16_t)(4U + 3U * count);
    if (length > PROTOCOL_FRAME_MAX) return FRAME_FATAL;
    if (available < length) return FRAME_NEED_MORE;
    *frame_length = length;
    if (xor_bytes(data, (uint16_t)(length - 1U)) != data[length - 1U]) return FRAME_DISCARD;
    return FRAME_READY;
  }
  if (data[0] == 0xCCU)
  {
    if (available < 2U) return FRAME_NEED_MORE;
    if ((data[1] == Set_Programmable_data) || (data[1] == Read_Programmable_data))
    {
      if (available < 8U) return FRAME_NEED_MORE;
      count = (uint16_t)((data[6] << 8) | data[7]);
      wide_length = (data[1] == Set_Programmable_data) ?
        (13U + 2U * (uint32_t)count) : 9U;
      if (wide_length > PROTOCOL_FRAME_MAX) return FRAME_FATAL;
      length = (uint16_t)wide_length;
      if (available < length) return FRAME_NEED_MORE;
      *frame_length = length;
      payload_length = (data[1] == Set_Programmable_data) ?
        (uint16_t)(length - 1U) : 8U;
      return (xor_bytes(data, payload_length) == data[length - 1U]) ? FRAME_READY : FRAME_DISCARD;
    }
    if (available < 7U) return FRAME_NEED_MORE;
    *frame_length = 7U;
    return (xor_bytes(data, 6U) == data[6]) ? FRAME_READY : FRAME_DISCARD;
  }
  if (data[0] == 0x72U)
  {
    if (available < 3U) return FRAME_NEED_MORE;
    if ((data[1] != 0x68U) || ((data[2] != 0xAAU) && (data[2] != 0xBBU)))
      return FRAME_DISCARD;
    length = (data[2] == 0xAAU) ? 6U : 4U;
    if (available < length) return FRAME_NEED_MORE;
    if (data[length - 1U] != 0x16U) return FRAME_DISCARD;
    *frame_length = length;
    return FRAME_READY;
  }

  if (data[0] == 'S')
  {
    if (available < 2U) return FRAME_NEED_MORE;
    terminator = find_terminator(data, available, 8U);
    if (terminator == 0) return FRAME_NEED_MORE;
    if (terminator < 0) return FRAME_DISCARD;
    *frame_length = (uint16_t)terminator;
    return valid_ascii_s(data, *frame_length) ? FRAME_READY : FRAME_DISCARD;
  }
  if (data[0] == 'T')
  {
    terminator = find_terminator(data, available, 4U);
    if (terminator == 0) return FRAME_NEED_MORE;
    if (terminator < 0) return FRAME_DISCARD;
    *frame_length = (uint16_t)terminator;
    return valid_ascii_t(data, *frame_length) ? FRAME_READY : FRAME_DISCARD;
  }
  if (data[0] == 'C')
  {
    if (available < 2U) return FRAME_NEED_MORE;
    if ((data[1] == 'S') && (available < 3U)) return FRAME_NEED_MORE;
    if ((available >= 3U) && (data[1] == 'S') && (data[2] == 'T'))
    {
      *frame_length = 3U;
      return FRAME_READY;
    }
    terminator = find_terminator(data, available, 4U);
    if (terminator == 0) return FRAME_NEED_MORE;
    if (terminator < 0) return FRAME_DISCARD;
    length = (uint16_t)terminator;
    if (((length == 3U) && (data[1] == 'T')) ||
        ((length == 4U) && (data[1] == 'T') && ((data[2] == '0') || (data[2] == '1'))))
    {
      *frame_length = length;
      return FRAME_READY;
    }
    *frame_length = length;
    return FRAME_DISCARD;
  }
  if (data[0] == 'D')
  {
    terminator = find_terminator(data, available, 8U);
    if (terminator == 0) return FRAME_NEED_MORE;
    if (terminator < 0) return FRAME_DISCARD;
    *frame_length = (uint16_t)terminator;
    return valid_ascii_d(data, *frame_length) ? FRAME_READY : FRAME_DISCARD;
  }
  if (data[0] == 'P')
  {
    terminator = find_terminator(data, available, 9U);
    if (terminator == 0) return FRAME_NEED_MORE;
    if (terminator < 0) return FRAME_DISCARD;
    *frame_length = (uint16_t)terminator;
    return valid_ascii_p(data, *frame_length) ? FRAME_READY : FRAME_DISCARD;
  }
  if ((data[0] == '$') || (data[0] == '@'))
  {
    terminator = find_terminator(data, available, (data[0] == '$') ? 24U : 8U);
    if (terminator == 0) return FRAME_NEED_MORE;
    if (terminator < 0) return FRAME_DISCARD;
    *frame_length = (uint16_t)terminator;
    return valid_ascii_extension(data, *frame_length) ? FRAME_READY : FRAME_DISCARD;
  }

  return FRAME_DISCARD;
}

static uint16_t read_be16(const uint8_t *data)
{
  return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static uint8_t command_requires_channel(uint8_t command)
{
  switch (command)
  {
    case Set_Brightness:
    case Set_Color_temperature:
    case Set_Pulse_width:
    case Set_Light_delay:
    case Set_Camera_delay:
    case Read_Brightness:
    case Read_Channel_Switch:
    case Read_Color_temperature:
    case Read_Pulse_width:
    case Read_Light_delay:
    case Read_Camera_delay:
    case Read_Input_triggers_number:
    case Read_LightOutput_triggers_number:
    case Read_CameraOutput_triggers_number:
    case Read_TemperatureThreshold:
      return 1U;
    default:
      return 0U;
  }
}

static uint8_t validate_frame(const uint8_t *data, uint16_t length)
{
  uint8_t command;
  uint8_t channel;
  uint8_t i;
  unsigned int maximum;
  uint16_t count;
  uint16_t value;

  if ((data[0] == 0x72U) && (length == 6U))
    return 0U; /* Upgrade requests are deliberately ignored. */
  if (data[0] == 0xCAU)
  {
    channel = data[2];
    if ((channel > CHANNEL_NUM) ||
        ((channel == 0U) && command_requires_channel(data[1])))
    {
      reply_error(data[0], data[1], channel, 2U);
      return 0U;
    }
  }
  if (data[0] == 0xCBU)
  {
    count = data[2];
    if ((count == 0U) || (count > CHANNEL_NUM))
    {
      reply_error(data[0], data[1], data[2], 3U);
      return 0U;
    }
    command = data[1];
    if ((command == Set_Brightness) || (command == Set_Channel_Switch) ||
        (command == Set_Color_temperature) || (command == Set_brightness_level) ||
        (command == Set_Pulse_width) || (command == Set_Light_delay) ||
        (command == Set_Camera_delay))
    {
      maximum = Get_Controller_Data_MAX(command);
      for (i = 0U; i < count; ++i)
      {
        channel = data[3U + 3U * i];
        value = read_be16(&data[4U + 3U * i]);
        if ((channel > CHANNEL_NUM) ||
            ((channel == 0U) && command_requires_channel(command)))
        {
          reply_error(data[0], command, channel, 2U);
          return 0U;
        }
        if (value > maximum)
        {
          reply_error(data[0], command, channel, 3U);
          return 0U;
        }
      }
    }
  }
  if (data[0] == 0xCCU)
  {
    command = data[1];
    if ((command == Set_Programmable_data) || (command == Read_Programmable_data))
    {
      uint8_t recipe = data[2];
      uint8_t source = data[3];
      uint16_t line = read_be16(&data[4]);
      count = read_be16(&data[6]);
      if ((recipe == 0U) || (recipe > RECIPE_SUM))
      {
        reply_error(data[0], command, 1U, 3U);
        return 0U;
      }
      if ((source == 0U) || (source > TriggerSource_SUM))
      {
        reply_error(data[0], command, 2U, 3U);
        return 0U;
      }
      if ((line == 0U) || (line > Line_SUM))
      {
        reply_error(data[0], command, 3U, 3U);
        return 0U;
      }
      if (count > CHANNEL_NUM)
      {
        reply_error(data[0], command, 4U, 3U);
        return 0U;
      }
      return 1U;
    }
    if ((command >= Set_Prog_Total_steps) && (command <= Set_Prog_ResetTime))
    {
      if ((data[2] == 0U) || (data[2] > RECIPE_SUM))
      {
        reply_error(data[0], command, 1U, 3U);
        return 0U;
      }
      if ((data[3] == 0U) || (data[3] > TriggerSource_SUM))
      {
        reply_error(data[0], command, 2U, 3U);
        return 0U;
      }
      if (read_be16(&data[4]) > Get_Controller_Prog_Data_MAX(command))
      {
        reply_error(data[0], command, 4U, 3U);
        return 0U;
      }
    }
    else if ((command >= Read_Prog_Total_steps) && (command <= Read_Prog_ResetTime))
    {
      if ((data[2] == 0U) || (data[2] > RECIPE_SUM))
      {
        reply_error(data[0], command, 1U, 3U);
        return 0U;
      }
      if ((data[3] == 0U) || (data[3] > TriggerSource_SUM))
      {
        reply_error(data[0], command, 2U, 3U);
        return 0U;
      }
    }
    else if ((command == Set_Prog_Reset_steps) || (command == Set_Prog_Erase_data) ||
             (command == Set_Prog_software_trigger))
    {
      if ((data[2] == 0U) || (data[2] > RECIPE_SUM))
      {
        reply_error(data[0], command, 1U, 3U);
        return 0U;
      }
      if ((data[3] == 0U) || (data[3] > TriggerSource_SUM))
      {
        reply_error(data[0], command, 2U, 3U);
        return 0U;
      }
    }
  }
  return 1U;
}

static uint8_t dispatch_frame(const uint8_t *data, uint16_t length)
{
  unsigned int reply_length;
  unsigned char *reply;

  if (!validate_frame(data, length))
    return 1U;
  if (data[0] == 0x72U)
  {
    if (length == 6U)
      return 1U;
  }
  protocol_prepare_frame();
  input_data((unsigned char *)data, length);
  analysis_command();
  reply = get_prepare_tx_buffer(&reply_length);
  if (reply_length > PROTOCOL_FRAME_MAX)
  {
    fatal_error = 1U;
    return 0U;
  }
  if ((reply_length != 0U) && !tx_push(reply, (uint16_t)reply_length))
  {
    fatal_error = 1U;
    return 0U;
  }
  return 1U;
}

void ch9121_protocol_init(void)
{
  init_protocol_para();
  ch9121_protocol_reset_session();
}

void ch9121_protocol_reset_session(void)
{
  rx_head = 0U;
  rx_count = 0U;
  tx_head = 0U;
  tx_count = 0U;
  wait_active = 0U;
  wait_started = 0U;
  fatal_error = 0U;
}

uint8_t ch9121_protocol_receive(const uint8_t *data, uint16_t length)
{
  uint16_t tail;
  uint16_t first;
  if ((data == NULL) || (length == 0U) || (length > PROTOCOL_RX_CAPACITY - rx_count))
    return 0U;
  tail = (uint16_t)((rx_head + rx_count) % PROTOCOL_RX_CAPACITY);
  first = (uint16_t)(PROTOCOL_RX_CAPACITY - tail);
  if (first > length)
    first = length;
  memcpy(&rx_buffer[tail], data, first);
  if (first < length)
    memcpy(rx_buffer, &data[first], (uint16_t)(length - first));
  rx_count = (uint16_t)(rx_count + length);
  return 1U;
}

uint16_t ch9121_protocol_rx_available(void)
{
  return (uint16_t)(PROTOCOL_RX_CAPACITY - rx_count);
}

uint16_t ch9121_protocol_tx_pending(void)
{
  return tx_count;
}

void ch9121_protocol_process(struct tcp_pcb *pcb)
{
  uint16_t frame_length;
  frame_status_t status;
  uint32_t now;
  uint16_t send_length;
  uint16_t contiguous;
  err_t result;

  if ((pcb == NULL) || fatal_error)
    return;
  now = sys_now();
  while ((rx_count != 0U) && (tx_free() >= PROTOCOL_FRAME_MAX))
  {
    contiguous = (rx_count > PROTOCOL_FRAME_MAX) ? PROTOCOL_FRAME_MAX : rx_count;
    rx_copy(0U, frame_buffer, contiguous);
    status = scan_frame(frame_buffer, contiguous, &frame_length);
    if (status == FRAME_NEED_MORE)
    {
      if (wait_active == 0U)
      {
        wait_active = 1U;
        wait_started = now;
      }
      if ((uint32_t)(now - wait_started) < PROTOCOL_PARTIAL_TIMEOUT)
        break;
      rx_drop(1U);
      ++g_ch9121_transport_diagnostics.errors;
      continue;
    }
    if (status == FRAME_FATAL)
    {
      fatal_error = 1U;
      ++g_ch9121_transport_diagnostics.errors;
      return;
    }
    if (status == FRAME_DISCARD)
    {
      rx_drop((frame_length == 0U) ? 1U : frame_length);
      ++g_ch9121_transport_diagnostics.errors;
      continue;
    }
    if (frame_length == 0U)
    {
      fatal_error = 1U;
      return;
    }
    if (!dispatch_frame(frame_buffer, frame_length))
    {
      rx_drop(frame_length);
      ++g_ch9121_transport_diagnostics.errors;
      continue;
    }
    rx_drop(frame_length);
  }

  while ((tx_count != 0U) && (tcp_sndbuf(pcb) != 0U))
  {
    send_length = tx_count;
    if (send_length > tcp_sndbuf(pcb))
      send_length = tcp_sndbuf(pcb);
    contiguous = (uint16_t)(PROTOCOL_TX_CAPACITY - tx_head);
    if (send_length > contiguous)
      send_length = contiguous;
    result = tcp_write(pcb, &tx_buffer[tx_head], send_length, TCP_WRITE_FLAG_COPY);
    if (result == ERR_MEM)
      break;
    if (result != ERR_OK)
    {
      fatal_error = 1U;
      ++g_ch9121_transport_diagnostics.errors;
      return;
    }
    tx_head = (uint16_t)((tx_head + send_length) % PROTOCOL_TX_CAPACITY);
    tx_count = (uint16_t)(tx_count - send_length);
    g_ch9121_transport_diagnostics.tx_bytes += send_length;
  }
  if (tcp_output(pcb) != ERR_OK)
    ++g_ch9121_transport_diagnostics.errors;
}

uint8_t ch9121_protocol_failed(void)
{
  return fatal_error;
}
