#include "protocol_public.h"

#define    FREE				0
#define    BUSY				1


//********Communication global variable**************//
UART_Prepare_Buf prepare_data2; 

UART_Prepare_Buf prepare_data; 
unsigned char prepare_tx_buffer[SUM_SIZE] = {0};
unsigned int prepare_tx_length = 0;

void BootLoader_Reset(void)
{
	//set_BootLoader_flag();
	//__set_FAULTMASK(1);
	//HAL_NVIC_SystemReset();
}

void Prog_Software_Trigger(unsigned char recipe,unsigned char TriggerSource)
{
	// receive softwre cmd handle
	Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe-1][TriggerSource-1]++;
	if(Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe-1][TriggerSource-1] > Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe-1][TriggerSource-1])
	{
		Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe-1][TriggerSource-1] = 1;
	}
}

void Software_Trigger_channel(unsigned char channel)
{
	Controller_Data_a.Public_Data.Input_triggers_number[channel-1]++;
	
}


//	Digital Controller Strobe Controller General Parameters callback
//  LED UI 
void Set_Controller_Data_Callback(unsigned char command,unsigned char channel)		
{
	switch (command)
	{
		case Set_Brightness:
			//
			break;
		case Set_Channel_Switch:
			break;
		case Set_Digital_trigger_mode:				//set TH TL
			Controller_Data_a.Public_Data.mode = 0; //conversion to normal mode
			break;
		case Set_Color_temperature:
			break;
		case Set_brightness_level:
			break;
		case Set_Pulse_width:
			break;
		case Set_Light_delay:
			break;
		case Set_Camera_delay:
			break;
		case Set_Trigger_cycle:
			break;
		case Set_trigger_mode:						//set TR0 TR1
			Controller_Data_a.Public_Data.mode = 0; //conversion to normal mode
			break;
		case Set_Camera_trigger_mode:
			break;
		case Set_Pulse_width_unit:
			break;
		case Set_Trigger_filtering:
			break;
		case Set_mode:
			break;
		case Set_Baud_rate:
			break;
		case Set_Clean_Input_Output_TrigNumber:
			break;
		case Set_softwareTrig:
			Software_Trigger_channel(channel);
			break;
		case Set_RestoreFactorySettings:
			Controller_Data_FactorySettings();
			break;
		case Set_DataSave:
			break;
		case Set_TemperatureThreshold:
			break;
		case Set_ExploreSlave:
			break;
		default:
			break;
	}
}

void Read_Controller_Data_Callback(unsigned char command,unsigned char channel)		
{
	switch (command)
	{
		case Read_Input_triggers_number:
			break;
		case Read_LightOutput_triggers_number:
			break;
		case Read_CameraOutput_triggers_number:
			break;
		default:
			break;
	}
}


void Set_Controller_ProgData_Callback(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned char Line_number)		
{
	switch (command)
	{
		case Set_Programmable_data:		//Each line is modified once
			//
			break;
		case Set_Prog_Total_steps:		//All programmable modifications have been completed
			break;
		case Set_Prog_Current_steps:
			break;
		case Read_Prog_Current_steps:
			break;
		case Set_Prog_Trigger_mode:
			break;
		case Set_Prog_trigger_interval:
			break;
		case Set_Prog_Camera_delay:
			break;
		case Set_Prog_Light_delay:
			break;
		case Set_Prog_Step_switch:
			break;
		case Set_Prog_Start_Steps:
			break;
		case Set_Prog_Stop_Steps:
			break;
		case Set_Prog_Camera_output:
			break;
		case Set_Prog_Reset_steps:
			break;
		case Set_Prog_Erase_data:
			break;
		case Set_Prog_software_trigger:
			Prog_Software_Trigger(recipe,TriggerSource);
			break;
		default:
			break;
	}
}



void init_protocol_para(void)
{
	unsigned int i = 0;
	
	prepare_data.available_length = SUM_SIZE;
	prepare_data.data_end_position = 0;
	prepare_data.data_start_position = 0;
	prepare_data.state = 0;

	for(i = 0; i < SUM_SIZE; i++)
	{
		prepare_data.rx_buffer[i] = 0;
	}

	prepare_tx_length = 0;
	for(i = 0; i < SUM_SIZE; i++)
	{
		prepare_tx_buffer[i] = 0;
	}

	Controller_Data_Monitor_init();
}

void input_data(unsigned char *data,unsigned int length)
{
	unsigned int j = 0;
	unsigned int now_position = 0;
	
	if(prepare_data.state == FREE)
	{
		if(prepare_data.available_length < length)
		{
			return ;  //导入的数据比剩下的空间大
		}
		now_position = prepare_data.data_end_position;
		for(j = 0; j < length; j++)
		{
			prepare_data.rx_buffer[j+now_position] = data[j];
		}
		prepare_data.data_end_position = now_position + length;
		prepare_data.available_length = prepare_data.available_length - length;
	}
}

void package_tx_buffer(unsigned char *buf,unsigned int len)
{
	unsigned int i = 0;

	for(i=0; i < len; i++)
	{
		prepare_tx_buffer[prepare_tx_length] = buf[i];
		prepare_tx_length++;
	}
}

unsigned char* get_prepare_tx_buffer(unsigned int *length)
{
	*length = prepare_tx_length;
	prepare_tx_length = 0; 		//数据已经被拿走

	return prepare_tx_buffer;
}

void analysis_command(void)
{
	unsigned int i = 0;
	unsigned int length = SUM_SIZE - prepare_data.available_length;

	if(length == 0){
		return;
	}
	prepare_tx_length = 0;
	prepare_data.state = BUSY;
	for(i = 0; i < length; i++)
	{
		i = Transfer_cmd_handle_func(prepare_data.rx_buffer, i);;
	}

	//发送数据
	for(i = 0; i < length; i++)
	{
		prepare_data.rx_buffer[i] = 0;
	}
	prepare_data.state = FREE;
	prepare_data.data_end_position = 0;
	prepare_data.available_length  = SUM_SIZE;
}



