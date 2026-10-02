#include "protocol_public.h"
#include "app_light.h"
#include "app_flash.h"
#include "app.h"


#define FREE					0
#define BUSY					1

#define BRIGHTNESS_LEVEL_999	1
#define BRIGHTNESS_LEVEL_255	0				

//********Communication global variable**************//
UART_Prepare_Buf prepare_data2; 

UART_Prepare_Buf prepare_data; 
unsigned char prepare_tx_buffer[SUM_SIZE] = {0};
unsigned int prepare_tx_length = 0;

void BootLoader_Reset(void)
{
	set_BootLoader_flag();
#if STM32
	__set_FAULTMASK(1);
	HAL_NVIC_SystemReset();
#endif
#if GD32
	DISABLE_INT();
	NVIC_SystemReset();
#endif
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
			set_light_value(channel-1, Controller_Data_a.Digital_data.brightness[channel-1]);
			
			break;
		case Set_Channel_Switch:
			if(channel == 0)
				set_all_light_enable_state(Controller_Data_a.Digital_data.Channel_Switch[0]);
			else
				set_light_enable_state(channel-1, Controller_Data_a.Digital_data.Channel_Switch[channel-1]);
			break;
		case Set_Digital_trigger_mode:				//set TH TL
			Controller_Data_a.Public_Data.mode = 0; //conversion to normal mode
			set_light_normal_mode_state(Controller_Data_a.Digital_data.Digital_trigger_mode);
			break;
		case Set_Color_temperature:
			break;
		case Set_brightness_level:
			if(Controller_Data_a.Digital_data.brightness_level == BRIGHTNESS_LEVEL_255) 
				set_light_brightness_range(BRIGHTNESS_255);
			else if(Controller_Data_a.Digital_data.brightness_level == BRIGHTNESS_LEVEL_999)
				set_light_brightness_range(BRIGHTNESS_999);
			break;
		case Set_Pulse_width:
			set_light_strobe_width(channel-1, Controller_Data_a.Strobe_data.Pulse_width[channel-1]);
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
			set_light_mode(Controller_Data_a.Public_Data.mode);
			break;
		case Set_Baud_rate:
			//set_uart_baudrate(Controller_Data_a.Public_Data.Baud_rate);
			set_232_baud_gear(Controller_Data_a.Public_Data.Baud_rate+1);
			bsp_rs232_reinit();
            bsp_SaveBaudData();
			break;

		case Set_Clean_Input_Output_TrigNumber:
			bsp_trig_clear_all();
			bsp_light_output_clear_all();
			bsp_cam_clear_all();
			break;
		case Set_softwareTrig:
			Software_Trigger_channel(channel);
			if(0==  channel)
				bsp_all_trig_soft_trig(Controller_Data_a.Public_Data.SoftTrig_Width[0]);
			else
				bsp_trig_soft_trig(channel - 1, Controller_Data_a.Public_Data.SoftTrig_Width[channel-1]);
			break;
		case Set_RestoreFactorySettings:
			Controller_Data_FactorySettings();	
			light_factory_reset();
			break;
		case Set_DataSave:
			bsp_prog_trig_save_to_flash();
			break;
		case Set_TemperatureThreshold:
			//set_light_alarm_temp_threshold(Controller_Data_a.Public_Data.TempeThreshold[channel-1]);
			break;
		case Set_ExploreSlave:
			//
			{
				extern void enable_found_slave_display(void);
				extern spi_master_status_t master_find_slave_ctrl(bool on_off);
				enable_found_slave_display();
				master_find_slave_ctrl(true);
			}
			break;
		case Set_SynMode:
			 set_programmable_sync_mode_vlaue(Controller_Data_a.Public_Data.Syn_mode);
			//if (get_programmable_sync_mode() == SYNC_MODE_MASTER_SYNC) {
				start_prog_sync();
			//}
			break;
		default:
			break;
	}
}

void Get_Controller_Data_Callback(unsigned char command,unsigned char channel)
{
	switch (command)
	{
		case Read_Brightness:
			Controller_Data_a.Digital_data.brightness[channel-1] = get_light_value(channel-1);
			break;
		case Read_Channel_Switch:
			Controller_Data_a.Digital_data.Channel_Switch[channel-1] = get_light_enable_state(channel-1); ;
			break;
		case Read_Digital_trigger_mode:
			Controller_Data_a.Digital_data.Digital_trigger_mode = get_light_normal_mode_state();
			break;
		case Read_Color_temperature:
			break;
		case Read_brightness_level:
			if(get_light_brightness_range() == BRIGHTNESS_255)
				Controller_Data_a.Digital_data.brightness_level = BRIGHTNESS_LEVEL_255;
			else if(get_light_brightness_range() == BRIGHTNESS_999)
				Controller_Data_a.Digital_data.brightness_level = BRIGHTNESS_LEVEL_999;
			break;
		
		case Read_Pulse_width:
			Controller_Data_a.Strobe_data.Pulse_width[channel-1] = get_light_strobe_width(channel-1);
			break;
		case Read_Light_delay:
			break;
		case Read_Camera_delay:
			break;
		case Read_Trigger_cycle:
			break;
		case Read_trigger_mode:
			break;
		case Read_Camera_trigger_mode:
			break;
		case Read_Pulse_width_unit:
			break;
		case Read_Trigger_filtering:
			break;
		case Read_mode:
			Controller_Data_a.Public_Data.mode = get_light_mode();
			break;
		case Read_Baud_rate:
			Controller_Data_a.Public_Data.Baud_rate = get_232_baud_gear() - 1; //0 4800 1 9600 
			break;
		case Read_Input_triggers_number:
			Controller_Data_a.Public_Data.Input_triggers_number[channel-1] = bsp_trig_get_count(channel-1);
			break;
		case Read_LightOutput_triggers_number:
			Controller_Data_a.Public_Data.LightOutput_triggers_number[channel-1] = bsp_light_output_get_count(channel-1);
			break;
		case Read_Number_of_channels:
			break;
		case Read_Controller_model:
			break;
		case Read_TemperatureThreshold:
			/*if(channel == 0)
				Controller_Data_a.Public_Data.TempeThreshold[0] = get_light_alarm_temp_threshold();
			else
				Controller_Data_a.Public_Data.TempeThreshold[channel-1] = get_light_alarm_temp_threshold();*/
			break;
		case Read_SlavesNumber:
				Controller_Data_a.Public_Data.SlavesNumb = get_found_slave_count();
			break;
		case Read_SynMode:
				Controller_Data_a.Public_Data.Syn_mode  = get_programmable_sync_mode_value();
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
			Controller_Data_a.Public_Data.CameraOutput_triggers_number[channel-1] = bsp_cam_get_count(channel-1);
			break;
		default:
			break;
	}
}
void Set_Controller_ProgData_Callback(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned char Line_number)		
{
	switch (command)
	{
		case Set_Programmable_data:	
		{
		    uint8_t r = recipe - 1;
            uint8_t t = TriggerSource - 1;
            uint8_t l = Line_number - 1;

			prog_table_row_t step_x  = {	Controller_Data_a.Programmable_Data.Prog_data[r][t][l][0],
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][1],
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][2],
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][3],
										#if (CHANNEL_NUM == 8) 
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][4],
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][5],
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][6],
											Controller_Data_a.Programmable_Data.Prog_data[r][t][l][7],
										#endif
											Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[r][t][l],
											0, 
											0};
					
			//bsp_prog_trig_set_table(Line_number-1,&step_x);
			if (Line_number-1 < bsp_prog_trig_get_totsl_steps()) {
    			bsp_prog_trig_set_table(Line_number-1, &step_x);
			}	
			break;
		}
		case Set_Prog_Total_steps:		//All programmable modifications have been completed
			bsp_prog_trig_set_total_steps(Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe-1][TriggerSource-1] );
			break;
		case Set_Prog_Current_steps:
			bsp_prog_trig_set_current_step(Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe-1][TriggerSource-1]);
			break;
		case Set_Prog_Trigger_mode:
			bsp_prog_trig_set_mode(Controller_Data_a.Programmable_Data.Prog_Trigger_mode[recipe-1][TriggerSource-1]);
			break;
		case Set_Prog_trigger_interval:
			bsp_prog_trig_set_interval(Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe-1][TriggerSource-1]);
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
		case Set_Prog_HardwareResetSwitch:
			break;
		case Set_Prog_AutoResetSwitch:
			break;
		case Set_Prog_ResetTime:
			break;
		case Set_Prog_Reset_steps:
			bsp_prog_trig_reset();
			break;
		case Set_Prog_Erase_data:
			//bsp_prog_trig_delete_table(Line_number-1);
			 //__disable_irq(); 
			bsp_prog_trig_data_erase();
			//__enable_irq();  
			bsp_prog_trig_init();
			break;
		case Set_Prog_software_trigger:
			Prog_Software_Trigger(recipe,TriggerSource);
			bsp_prog_trig_soft_trigger();
			break;
		default:
			break;
	}
}

void Get_Controller_ProgData_Callback(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned char Line_number)		
{
	uint8_t r = recipe - 1;
    uint8_t t = TriggerSource - 1;
    uint8_t l = Line_number - 1;

	switch (command)
	{
		case Read_Programmable_data:	
		
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][0] = g_light_system.prog_table[l].brightness[0];
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][1] = g_light_system.prog_table[l].brightness[1];
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][2] = g_light_system.prog_table[l].brightness[2];
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][3] = g_light_system.prog_table[l].brightness[3];
		#if (CHANNEL_NUM == 8) 
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][4] = g_light_system.prog_table[l].brightness[4];
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][5] = g_light_system.prog_table[l].brightness[5];
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][6] = g_light_system.prog_table[l].brightness[6];
		
			Controller_Data_a.Programmable_Data.Prog_data[r][t][l][7] = g_light_system.prog_table[l].brightness[7];
		#endif
			Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[r][t][l] = g_light_system.prog_table[l].prog_strobe_width;
		break;
		
		case Read_Prog_Total_steps:		//All programmable modifications have been completed
			Controller_Data_a.Programmable_Data.Prog_Total_steps[r][t] = bsp_prog_trig_get_totsl_steps();
			break;
		case Read_Prog_Current_steps:
			Controller_Data_a.Programmable_Data.Prog_Current_steps[r][t] = bsp_prog_trig_get_current_step() + 1;
			break;
		case Read_Prog_Trigger_mode:
			Controller_Data_a.Programmable_Data.Prog_Trigger_mode[r][t]	= bsp_prog_trig_get_mode();
			break;
		case Read_Prog_trigger_interval:
			Controller_Data_a.Programmable_Data.Prog_trigger_interval[r][t] = bsp_prog_trig_get_interval();
			break;
		case Read_Prog_Camera_delay:
			break;
		case Read_Prog_Light_delay:
			break;
		case Read_Prog_Step_switch:
			break;
		case Read_Prog_Start_Steps:
			break;
		case Read_Prog_Stop_Steps:
			break;
		case Read_Prog_Camera_output:
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



