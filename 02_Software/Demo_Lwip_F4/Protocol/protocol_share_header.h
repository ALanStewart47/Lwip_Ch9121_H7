#ifndef __PROTOCOL_SHARE_HEADER_H
#define __PROTOCOL_SHARE_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "protocol_public.h"

typedef enum
{
	brightness_MAX_1 = 255,
	brightness_MAX_2 = 999,
	Channel_Switch_MAX = 1,
	Digital_trigger_mode_MAX = 1,		//1常亮/0常灭
	Color_temperature_MAX = 255,		
	brightness_level_MAX = 1			//1 999/0 255
} Digital_Parameter_Max;	

typedef enum
{
	Pulse_width_MAX = 999,
	Light_delay_MAX = 999,
	Camera_delay_MAX = 999,
	Trigger_cycle_MAX = 999,
	trigger_mode_MAX = 2,
	Camera_trigger_mode_MAX = 1,
	Pulse_width_unit_MAX = 1,
	Trigger_filtering_MAX = 999
} Strobe_Parameter_Max;	

typedef enum
{
	mode_MAX = 2,
	Baud_rate_MAX = 7,
	SYNmode_MAX = 1
} Public_Parameter_Max;	


typedef enum
{
	Prog_value_MAX = 999,		//Digital 255 Strobe 999
	Prog_Pulse_width_MAX = 999,		//Digital
	Prog_Trigger_mode_MAX = 1,
	Prog_trigger_interval_MAX = 999,
	Prog_Camera_delay_MAX = 999,
	Prog_Light_delay_MAX = 999,
	Prog_Step_switch_MAX = 1,
	Prog_Camera_output_MAX = 1,
	Prog_HardwareResetSwitch_MAX = 1,
	Prog_AutoResetSwitch_MAX = 1,
	Prog_ResetTime_MAX = 999
} Programmable_Parameter_Max;	



#define		SX0XXX_ENABLE			1
#define		SPX0XXX_ENABLE			1
#define		SPUXX_ENABLE			1
#define		ST0XXX_ENABLE			1
#define		SXXXXX_ENABLE			1
#define		SWXXXX_ENABLE			1
#define		SAVE_ENABLE				1
#define		SWTRIG_ENABLE			1

#define		TRX_ENABLE				0
#define		TX_ENABLE				1
#define		TPX_ENABLE				1

#define		DLX0XXX_ENABLE			1
#define		DCX0XXX_ENABLE			1
#define		DC0XXX_ENABLE			1

#define		CTX_ENABLE				1

#define		PRCL_ENABLE				1
#define		PRWX_ENABLE				1
#define		PRNXXX_ENABLE			1
#define		PRENCLRX_ENABLE			1

typedef enum
{
	Digital = 1,
	Strobe,
	ModulDigital,
	ModulStrobe
} controller;		//controller type

typedef enum
{
	Set_Brightness = 0x01,
	Set_Channel_Switch,
	Set_Digital_trigger_mode,		//常亮/常灭
	Set_Color_temperature,
	Set_brightness_level
} Digital_Command_Set;	
	
typedef enum
{
	Read_Brightness = 0x61,
	Read_Channel_Switch,
	Read_Digital_trigger_mode,
	Read_Color_temperature,
	Read_brightness_level
} Digital_Command_Read;	

typedef enum
{
	Set_Pulse_width = 0x21,
	Set_Light_delay,
	Set_Camera_delay,
	Set_Trigger_cycle,
	Set_trigger_mode,
	Set_Camera_trigger_mode,
	Set_Pulse_width_unit,
	Set_Trigger_filtering
} Strobe_Command_Set;	
	
typedef enum
{
	Read_Pulse_width = 0x81,
	Read_Light_delay,
	Read_Camera_delay,
	Read_Trigger_cycle,
	Read_trigger_mode,
	Read_Camera_trigger_mode,
	Read_Pulse_width_unit,
	Read_Trigger_filtering
} Strobe_Command_Read;	

typedef enum
{
	Set_mode = 0x41,
	Set_Baud_rate,
	Set_Clean_Input_Output_TrigNumber,
	Set_NULL,
	Set_softwareTrig,
	Set_RestoreFactorySettings,
	Set_DataSave,
	Set_TemperatureThreshold,
	Set_ExploreSlave,
	Set_SynMode
} Public_Command_Set;	
	
typedef enum
{
	Read_mode = 0xA1,
	Read_Baud_rate,
	Read_Input_triggers_number,
	Read_LightOutput_triggers_number,
	Read_Number_of_channels,
	Read_Controller_model,
	Read_CameraOutput_triggers_number,
	Read_TemperatureThreshold,
	Read_SlavesNumber,
	Read_SynMode
} Public_Command_Read;	
	

typedef enum
{
	Set_Programmable_data = 0x01,
	Set_Prog_Total_steps,
	Set_Prog_Current_steps,
	Set_Prog_Trigger_mode,		//单步/连续
	Set_Prog_trigger_interval,
	Set_Prog_Camera_delay,
	Set_Prog_Light_delay,
	Set_Prog_Step_switch,
	Set_Prog_Start_Steps,
	Set_Prog_Stop_Steps,
	Set_Prog_Camera_output,
	Set_Prog_HardwareResetSwitch,
	Set_Prog_AutoResetSwitch,
	Set_Prog_ResetTime,
	Set_Prog_Reset_steps = 0x51,
	Set_Prog_Erase_data,
	Set_Prog_software_trigger
} Programmable_Command_Set;	

typedef enum
{
	Read_Programmable_data = 0x31,
	Read_Prog_Total_steps,
	Read_Prog_Current_steps,
	Read_Prog_Trigger_mode,		//单步/连续
	Read_Prog_trigger_interval,
	Read_Prog_Camera_delay,
	Read_Prog_Light_delay,
	Read_Prog_Step_switch,
	Read_Prog_Start_Steps,
	Read_Prog_Stop_Steps,
	Read_Prog_Camera_output,
	Read_Prog_HardwareResetSwitch,
	Read_Prog_AutoResetSwitch,
	Read_Prog_ResetTime,
	Read_Prog_Command_Max
} Programmable_Command_Read;	


void Controller_Data_Monitor_init(void);
void Controller_Data_FactorySettings(void);

void BootLoader_Reset(void);

void package_tx_buffer(unsigned char *buf,unsigned int len);


//**************Data processing functions*********************************************//
unsigned int Get_Controller_Data_MAX(unsigned char command);
unsigned int Get_Controller_Prog_Data_MAX(unsigned char command);

void Set_Controller_Data_Callback(unsigned char command,unsigned char channel)	;
void Read_Controller_Data_Callback(unsigned char command,unsigned char channel)	;
void Get_Controller_Data_Callback(unsigned char command,unsigned char channel);
void Set_Controller_ProgData_Callback(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned char Line_number);
void Get_Controller_ProgData_Callback(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned char Line_number);
void Software_Trigger(unsigned char recipe,unsigned char TriggerSource);


void Set_Controller_Data(unsigned char command,unsigned char channel,unsigned int data);
unsigned int Get_Controller_Data(unsigned char command,unsigned char channel);

void Set_Controller_Prog_Data(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned int data);
unsigned int Get_Controller_Prog_Data(unsigned char command,unsigned char recipe,unsigned char TriggerSource);

void Set_Controller_Prog_TableData(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned char channel_number,unsigned int data);
unsigned int Get_Controller_Prog_TableData(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned char channel_number);

void Set_Controller_Prog_TablePulseWidth(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned int data);
unsigned int Get_Controller_Prog_TablePulseWidth(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number)	;

void Set_Controller_Prog_TableCameraOutput(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned char data);
unsigned char Get_Controller_Prog_TableCameraOutput(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number);


void Erasing_Programmable_Data(unsigned char recipe,unsigned int TriggerSource);



//**************New protocol processing function**************************************//
unsigned int New_cmd_handle_func(unsigned char *uart_buf,unsigned int i);


//**************Old protocol processing function**************************************//
unsigned int Extend_write_cmdhandle(unsigned char *uart_buf,unsigned int i);
unsigned int Extend_read_cmdhandle(unsigned char *uart_buf,unsigned int i);

unsigned int Old_cmd_handle_func(unsigned char *uart_buf,unsigned int i);


//**************Protocol Transfer Function********************************************//
unsigned int Transfer_cmd_handle_func(unsigned char *uart_buf,unsigned int i);


#ifdef __cplusplus
}
#endif

#endif 
