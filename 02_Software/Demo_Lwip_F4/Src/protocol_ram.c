#include "protocol_public.h"
#include "protocol_ram.h"

#include <string.h>

UART_Prepare_Buf prepare_data2;
unsigned char prepare_tx_buffer[SUM_SIZE];
unsigned int prepare_tx_length;
unsigned char cmd_up[15];

static UART_Prepare_Buf prepare_data;

static uint8_t channel_valid(unsigned char channel)
{
  return (uint8_t)((channel >= 1U) && (channel <= CHANNEL_NUM));
}

void BootLoader_Reset(void)
{
  /* Firmware update and reset are disabled in this TCP-only port. */
}

void init_protocol_para(void)
{
  memset(&prepare_data, 0, sizeof(prepare_data));
  memset(&prepare_data2, 0, sizeof(prepare_data2));
  memset(prepare_tx_buffer, 0, sizeof(prepare_tx_buffer));
  memset(cmd_up, 0, sizeof(cmd_up));
  prepare_data.available_length = SUM_SIZE;
  prepare_tx_length = 0U;

  memset(&Controller_Data_a, 0, sizeof(Controller_Data_a));
  Controller_Data_FactorySettings();
  Controller_Data_Monitor_init();
}

void input_data(unsigned char *data, unsigned int length)
{
  if ((data == NULL) || (length == 0U) ||
      (length > prepare_data.available_length))
    return;

  memcpy(&prepare_data.rx_buffer[prepare_data.data_end_position], data, length);
  prepare_data.data_end_position += length;
  prepare_data.available_length -= length;
}

void protocol_prepare_frame(void)
{
  memset(&prepare_data, 0, sizeof(prepare_data));
  memset(prepare_tx_buffer, 0, sizeof(prepare_tx_buffer));
  prepare_data.available_length = SUM_SIZE;
  prepare_tx_length = 0U;
}

void package_tx_buffer(unsigned char *data, unsigned int length)
{
  if ((data == NULL) || (length > SUM_SIZE - prepare_tx_length))
    return;

  memcpy(&prepare_tx_buffer[prepare_tx_length], data, length);
  prepare_tx_length += length;
}

unsigned char *get_prepare_tx_buffer(unsigned int *length)
{
  if (length != NULL)
  {
    *length = prepare_tx_length;
    prepare_tx_length = 0U;
  }
  return prepare_tx_buffer;
}

void analysis_command(void)
{
  unsigned int length = SUM_SIZE - prepare_data.available_length;

  if (length == 0U)
    return;

  prepare_data.state = 1U;
  prepare_tx_length = 0U;
  (void)Transfer_cmd_handle_func(prepare_data.rx_buffer, 0U);

  memset(prepare_data.rx_buffer, 0, length);
  prepare_data.state = 0U;
  prepare_data.data_end_position = 0U;
  prepare_data.available_length = SUM_SIZE;
}

void Set_Controller_Data_Callback(unsigned char command, unsigned char channel)
{
  unsigned char index;

  switch (command)
  {
    case Set_RestoreFactorySettings:
      memset(&Controller_Data_a, 0, sizeof(Controller_Data_a));
      Controller_Data_FactorySettings();
      Controller_Data_Monitor_init();
      break;

    case Set_Clean_Input_Output_TrigNumber:
      memset(Controller_Data_a.Public_Data.Input_triggers_number, 0,
             sizeof(Controller_Data_a.Public_Data.Input_triggers_number));
      memset(Controller_Data_a.Public_Data.LightOutput_triggers_number, 0,
             sizeof(Controller_Data_a.Public_Data.LightOutput_triggers_number));
      memset(Controller_Data_a.Public_Data.CameraOutput_triggers_number, 0,
             sizeof(Controller_Data_a.Public_Data.CameraOutput_triggers_number));
      break;

    case Set_softwareTrig:
      if (channel == 0U)
      {
        for (index = 0U; index < CHANNEL_NUM; ++index)
          ++Controller_Data_a.Public_Data.Input_triggers_number[index];
      }
      else if (channel_valid(channel))
      {
        ++Controller_Data_a.Public_Data.Input_triggers_number[channel - 1U];
      }
      break;

    case Set_ExploreSlave:
      Controller_Data_a.Public_Data.SlavesNumb = 0U;
      break;

    default:
      /* Save confirms in the protocol parser but has no flash side effect. */
      break;
  }
}

void Get_Controller_Data_Callback(unsigned char command, unsigned char channel)
{
  (void)command;
  (void)channel;
  Controller_Data_a.Public_Data.SlavesNumb = 0U;
}

void Read_Controller_Data_Callback(unsigned char command, unsigned char channel)
{
  (void)command;
  (void)channel;
}

void Set_Controller_ProgData_Callback(unsigned char command, unsigned char recipe,
                                      unsigned char source, unsigned char line)
{
  (void)line;
  if ((recipe == 0U) || (recipe > RECIPE_SUM) || (source == 0U) ||
      (source > TriggerSource_SUM))
    return;

  if (command == Set_Prog_software_trigger)
    Software_Trigger(recipe, source);
}

void Get_Controller_ProgData_Callback(unsigned char command, unsigned char recipe,
                                      unsigned char source, unsigned char line)
{
  (void)command;
  (void)recipe;
  (void)source;
  (void)line;
}

void Software_Trigger(unsigned char recipe, unsigned char source)
{
  unsigned int *current;
  unsigned int total;

  if ((recipe == 0U) || (recipe > RECIPE_SUM) || (source == 0U) ||
      (source > TriggerSource_SUM))
    return;

  current = &Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe - 1U][source - 1U];
  total = Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe - 1U][source - 1U];
  if (total != 0U)
    *current = (*current >= total) ? 1U : (*current + 1U);
}

void clear_tx_buff(unsigned int length)
{
  if (length > SUM_SIZE)
    length = SUM_SIZE;
  memset(prepare_tx_buffer, 0, length);
  prepare_tx_length = 0U;
}

void set_state_clean(unsigned char channel, unsigned int value)
{
  Set_Controller_Data(Set_Channel_Switch, channel, value);
}

void set_state_clean_(unsigned char channel, unsigned int value)
{
  set_state_clean(channel, value);
}

void f3_data_up(void)
{
}
