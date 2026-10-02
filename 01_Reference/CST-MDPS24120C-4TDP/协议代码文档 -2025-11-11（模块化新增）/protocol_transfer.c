#include "protocol_public.h"

//********CSoftware version**************//
unsigned char version[2] = {1,0};
unsigned char controller_model[2] = {Strobe,CHANNEL_NUM};	//1-Digital 2-Strobe 3-other + channl sum

Controller_Data Controller_Data_a;

void Controller_Data_Monitor_init(void)
{
	int channel = 0;
	int recipe = 0;
	int TriggerSource = 0;
	int Line = 0;

	if(controller_model[0] == Digital)
	{
		if(Controller_Data_a.Digital_data.Digital_trigger_mode > Digital_trigger_mode_MAX)
				Controller_Data_a.Digital_data.Digital_trigger_mode = 0;
		if(Controller_Data_a.Digital_data.brightness_level > brightness_level_MAX)
				Controller_Data_a.Digital_data.brightness_level = 0;
		int max = 255;
		if(Controller_Data_a.Digital_data.brightness_level == 0)
		{
			max = brightness_MAX_1;
		}else
		{
			max = brightness_MAX_2;
		}
		for(channel = 0; channel < CHANNEL_NUM; channel++)
		{
			if(Controller_Data_a.Digital_data.brightness[channel] > max)
				Controller_Data_a.Digital_data.brightness[channel] = 0;
			if(Controller_Data_a.Digital_data.Channel_Switch[channel] > Channel_Switch_MAX)
				Controller_Data_a.Digital_data.Channel_Switch[channel] = 0;
			if(Controller_Data_a.Strobe_data.Pulse_width[channel] > Pulse_width_MAX)
				Controller_Data_a.Strobe_data.Pulse_width[channel] = 0;
		}
	}
	else if(controller_model[0] == Strobe)
	{
		if(Controller_Data_a.Strobe_data.Trigger_cycle > Trigger_cycle_MAX)
				Controller_Data_a.Strobe_data.Trigger_cycle = 15;
		if(Controller_Data_a.Strobe_data.trigger_mode > trigger_mode_MAX)
				Controller_Data_a.Strobe_data.trigger_mode = 0;
		if(Controller_Data_a.Strobe_data.Camera_trigger_mode > Camera_trigger_mode_MAX)
				Controller_Data_a.Strobe_data.Camera_trigger_mode = 0;
		if(Controller_Data_a.Strobe_data.Pulse_width_unit > Pulse_width_unit_MAX)
				Controller_Data_a.Strobe_data.Pulse_width_unit = 0;
		if(Controller_Data_a.Strobe_data.Trigger_filtering > Trigger_filtering_MAX)
				Controller_Data_a.Strobe_data.Trigger_filtering = 0;
		for(channel = 0; channel < CHANNEL_NUM; channel++)
		{
			if(Controller_Data_a.Strobe_data.Pulse_width[channel] > Pulse_width_MAX)
				Controller_Data_a.Strobe_data.Pulse_width[channel] = 0;
			if(Controller_Data_a.Strobe_data.Light_delay[channel] > Light_delay_MAX)
				Controller_Data_a.Strobe_data.Light_delay[channel] = 0;
			if(Controller_Data_a.Strobe_data.Camera_delay[channel] > Camera_delay_MAX)
				Controller_Data_a.Strobe_data.Camera_delay[channel] = 0;
		}
	}
	if(Controller_Data_a.Public_Data.mode > mode_MAX)
		Controller_Data_a.Public_Data.mode = 0;
	if(Controller_Data_a.Public_Data.Baud_rate > Baud_rate_MAX)
		Controller_Data_a.Public_Data.Baud_rate = 3;

	for(recipe = 0; recipe < RECIPE_SUM; recipe++)
	{
		for(TriggerSource = 0; TriggerSource < TriggerSource_SUM; TriggerSource++)
		{
			if(Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource] > Line_SUM)
				Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource] = 0;
			if(Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe][TriggerSource] > Line_SUM)
				Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe][TriggerSource] = 0;
			if(Controller_Data_a.Programmable_Data.Prog_Trigger_mode[recipe][TriggerSource] > Prog_Trigger_mode_MAX)
				Controller_Data_a.Programmable_Data.Prog_Trigger_mode[recipe][TriggerSource] = 0;
			if(Controller_Data_a.Programmable_Data.Prog_Camera_delay[recipe][TriggerSource] > Prog_Camera_delay_MAX)
				Controller_Data_a.Programmable_Data.Prog_Camera_delay[recipe][TriggerSource] = 0;
			if(Controller_Data_a.Programmable_Data.Prog_Light_delay[recipe][TriggerSource] > Prog_Light_delay_MAX)
				Controller_Data_a.Programmable_Data.Prog_Light_delay[recipe][TriggerSource] = 0;
			if(Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource] > Prog_trigger_interval_MAX)
			{
				if(controller_model[0] == Strobe)		//strobe  trigger_interval mindata is 1 / Digital 0
				{
					Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource] = 1;
				}else
				{
					Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource] = 0;
				}
			}
			for(Line = 0; Line < Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource]; Line++)
			{
				for(channel = 0; channel < CHANNEL_NUM; channel++)
				{
					if(Controller_Data_a.Programmable_Data.Prog_data[recipe][TriggerSource][Line][channel]>Prog_value_MAX)
						Controller_Data_a.Programmable_Data.Prog_data[recipe][TriggerSource][Line][channel] = 0;
				}
				if(Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[recipe][TriggerSource][Line]>Prog_Pulse_width_MAX)
						Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[recipe][TriggerSource][Line] = 0;
			}
		}
	}

}

void Controller_Data_FactorySettings(void)
{
	int channel = 0;
	int recipe = 0;
	int TriggerSource = 0;
	int Line = 0;

	if(controller_model[0] == Digital)
	{
		Controller_Data_a.Digital_data.Digital_trigger_mode = 0;
		Controller_Data_a.Digital_data.brightness_level = 0;
		for(channel = 0; channel < CHANNEL_NUM; channel++)
		{
			Controller_Data_a.Digital_data.brightness[channel] = 0;
			Controller_Data_a.Digital_data.Channel_Switch[channel] = 0;
			Controller_Data_a.Strobe_data.Pulse_width[channel] = 0;
		}
	}
	else if(controller_model[0] == Strobe)
	{
		Controller_Data_a.Strobe_data.Trigger_cycle = 15;
		Controller_Data_a.Strobe_data.trigger_mode = 0;
		Controller_Data_a.Strobe_data.Camera_trigger_mode = 0;
		Controller_Data_a.Strobe_data.Pulse_width_unit = 0;
		Controller_Data_a.Strobe_data.Trigger_filtering = 0;
		for(channel = 0; channel < CHANNEL_NUM; channel++)
		{
			Controller_Data_a.Strobe_data.Pulse_width[channel] = 0;
			Controller_Data_a.Strobe_data.Light_delay[channel] = 0;
			Controller_Data_a.Strobe_data.Camera_delay[channel] = 0;
		}
	}
	Controller_Data_a.Public_Data.mode = 0;
	Controller_Data_a.Public_Data.Baud_rate = 3;

	for(recipe = 0; recipe < RECIPE_SUM; recipe++)
	{
		for(TriggerSource = 0; TriggerSource < TriggerSource_SUM; TriggerSource++)
		{
			Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource] = 0;
			Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe][TriggerSource] = 0;
			Controller_Data_a.Programmable_Data.Prog_Trigger_mode[recipe][TriggerSource] = 0;
			Controller_Data_a.Programmable_Data.Prog_Camera_delay[recipe][TriggerSource] = 0;
			Controller_Data_a.Programmable_Data.Prog_Light_delay[recipe][TriggerSource] = 0;
			if(controller_model[0] == Strobe)		//strobe  trigger_interval mindata is 1 / Digital 0
			{
				Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource] = 1;
			}else
			{
				Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource] = 0;
			}
			for(Line = 0; Line < Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource]; Line++)
			{
				for(channel = 0; channel < CHANNEL_NUM; channel++)
				{
					Controller_Data_a.Programmable_Data.Prog_data[recipe][TriggerSource][Line][channel] = 0;
				}
				Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[recipe][TriggerSource][Line] = 0;
				Controller_Data_a.Programmable_Data.Prog_Camera_output[recipe][TriggerSource][Line] = 0;
			}
		}
	}
}


unsigned int Get_Controller_Data_MAX(unsigned char command)
{
	int data = 0;
	
	switch (command)
	{
		case Set_Brightness:
			if(Controller_Data_a.Digital_data.brightness_level == 0)
			{
				data = brightness_MAX_1;
			}else
			{
				data = brightness_MAX_2;
			}
			break;
		case Set_Channel_Switch:
			data = Channel_Switch_MAX;
			break;
		case Set_Digital_trigger_mode:
			data = Digital_trigger_mode_MAX;
			break;
		case Set_Color_temperature:
			data = Color_temperature_MAX;
			break;
		case Set_brightness_level:
			data = brightness_level_MAX;
			break;
		
		case Set_Pulse_width:
			data = Pulse_width_MAX;
			break;
		case Set_Light_delay:
			data = Light_delay_MAX;
			break;
		case Set_Camera_delay:
			data = Camera_delay_MAX;
			break;
		case Set_Trigger_cycle:
			data = Trigger_cycle_MAX;
			break;
		case Set_trigger_mode:
			data = trigger_mode_MAX;
			break;
		case Set_Camera_trigger_mode:
			data = Camera_trigger_mode_MAX;
			break;
		case Set_Pulse_width_unit:
			data = Pulse_width_unit_MAX;
			break;
		case Set_Trigger_filtering:
			data = Trigger_filtering_MAX;
			break;

		case Set_mode:
			data = mode_MAX;
			break;
		case Set_Baud_rate:
			data = Baud_rate_MAX;
			break;
		default:
			data = 999;
			break;
	}
	return data;
}

unsigned int Get_Controller_Prog_Data_MAX(unsigned char command)
{
	int data = 0;
	
	switch (command)
	{
		case Set_Prog_Total_steps:
		case Set_Prog_Current_steps:
		case Set_Prog_Stop_Steps:
		case Set_Prog_Start_Steps:
			data = Line_SUM;
			break;
		case Set_Prog_Trigger_mode:
			data = Prog_Trigger_mode_MAX;
			break;
		case Set_Prog_trigger_interval:
			data = Prog_trigger_interval_MAX;
			break;
		case Set_Prog_Camera_delay:
			data = Prog_Camera_delay_MAX;
			break;
		case Set_Prog_Light_delay:
			data = Prog_Light_delay_MAX;
			break;
		case Set_Prog_Step_switch:
			data = Prog_Step_switch_MAX;
			break;
		case Set_Prog_Camera_output:
			data = Prog_Camera_output_MAX;
			break;
		default:
			data = 999;
			break;
	}
	return data;
}


void Set_Controller_Data(unsigned char command,unsigned char channel,unsigned int data)		
{
	if((command == Set_Brightness)||(command == Set_Color_temperature)||(command == Set_Pulse_width)
		||(command == Set_Light_delay)||(command == Set_Camera_delay))
	{
		if(channel == 0)		//Prevent channel 0 array overflow
			return;
	}
	switch (command)
	{
		case Set_Brightness:
			Controller_Data_a.Digital_data.brightness[channel-1] = data;
			break;
		case Set_Channel_Switch:
			if(channel == 0)
			{
				Controller_Data_a.Digital_data.Channel_Switch[0] = data;
			}else
			{
				Controller_Data_a.Digital_data.Channel_Switch[channel-1] = data;
			}
			break;
		case Set_Digital_trigger_mode:
			Controller_Data_a.Digital_data.Digital_trigger_mode = data;
			break;
		case Set_Color_temperature:
			Controller_Data_a.Digital_data.Color_Temperature[channel-1] = data;
			break;
		case Set_brightness_level:
			Controller_Data_a.Digital_data.brightness_level = data;
			break;
		
		case Set_Pulse_width:
			Controller_Data_a.Strobe_data.Pulse_width[channel-1] = data;
			break;
		case Set_Light_delay:
			Controller_Data_a.Strobe_data.Light_delay[channel-1] = data;
			break;
		case Set_Camera_delay:
			Controller_Data_a.Strobe_data.Camera_delay[channel-1] = data;
			break;
		case Set_Trigger_cycle:
			Controller_Data_a.Strobe_data.Trigger_cycle = data;
			break;
		case Set_trigger_mode:
			Controller_Data_a.Strobe_data.trigger_mode = data;
			break;
		case Set_Camera_trigger_mode:
			Controller_Data_a.Strobe_data.Camera_trigger_mode = data;
			break;
		case Set_Pulse_width_unit:
			Controller_Data_a.Strobe_data.Pulse_width_unit = data;
			break;
		case Set_Trigger_filtering:
			Controller_Data_a.Strobe_data.Trigger_filtering = data;
			break;

		case Set_mode:
			Controller_Data_a.Public_Data.mode = data;
			break;
		case Set_Baud_rate:
			Controller_Data_a.Public_Data.Baud_rate = data;
			break;
		case Set_softwareTrig:
			if(channel == 0)
			{
				Controller_Data_a.Public_Data.SoftTrig_Width[0] = data;
			}else
			{
				Controller_Data_a.Public_Data.SoftTrig_Width[channel-1] = data;
			}
			break;
		case Set_Clean_Input_Output_TrigNumber:
			//Controller_Data_a.Public_Data.Input_triggers_number = 0;
			break;
		case Set_TemperatureThreshold:
			if(channel == 0)
			{
				Controller_Data_a.Public_Data.TempeThreshold[0] = data;
			}else
			{
				Controller_Data_a.Public_Data.TempeThreshold[channel-1] = data;
			}
			break;
		case Set_ExploreSlave:

			break;
		default:
			break;
	}
	Set_Controller_Data_Callback(command,channel);
}

unsigned int Get_Controller_Data(unsigned char command,unsigned char channel)
{
	int data = 0;

	Read_Controller_Data_Callback(command,channel);
	switch (command)
	{
		case Read_Brightness:
			data = Controller_Data_a.Digital_data.brightness[channel-1];
			break;
		case Read_Channel_Switch:
			data = Controller_Data_a.Digital_data.Channel_Switch[channel-1];
			break;
		case Read_Digital_trigger_mode:
			data = Controller_Data_a.Digital_data.Digital_trigger_mode;
			break;
		case Read_Color_temperature:
			data = Controller_Data_a.Digital_data.Color_Temperature[channel-1];
			break;
		case Read_brightness_level:
			data = Controller_Data_a.Digital_data.brightness_level;
			break;
		
		case Read_Pulse_width:
			data = Controller_Data_a.Strobe_data.Pulse_width[channel-1];
			break;
		case Read_Light_delay:
			data = Controller_Data_a.Strobe_data.Light_delay[channel-1];
			break;
		case Read_Camera_delay:
			data = Controller_Data_a.Strobe_data.Camera_delay[channel-1];
			break;
		case Read_Trigger_cycle:
			data = Controller_Data_a.Strobe_data.Trigger_cycle;
			break;
		case Read_trigger_mode:
			data = Controller_Data_a.Strobe_data.trigger_mode;
			break;
		case Read_Camera_trigger_mode:
			data = Controller_Data_a.Strobe_data.Camera_trigger_mode;
			break;
		case Read_Pulse_width_unit:
			data = Controller_Data_a.Strobe_data.Pulse_width_unit;
			break;
		case Read_Trigger_filtering:
			data = Controller_Data_a.Strobe_data.Trigger_filtering;
			break;

		case Read_mode:
			data = Controller_Data_a.Public_Data.mode;
			break;
		case Read_Baud_rate:
			data = Controller_Data_a.Public_Data.Baud_rate;
			break;
		case Read_Input_triggers_number:
			data = Controller_Data_a.Public_Data.Input_triggers_number[channel-1];
			break;
		case Read_LightOutput_triggers_number:
			data = Controller_Data_a.Public_Data.LightOutput_triggers_number[channel-1];
			break;
		case Read_CameraOutput_triggers_number:
			data = Controller_Data_a.Public_Data.CameraOutput_triggers_number[channel-1];
			break;
		case Read_Number_of_channels:
			data = CHANNEL_NUM;
			break;
		case Read_Controller_model:
			//data = Digital;
			data = controller_model[0];
			break;
		case Read_TemperatureThreshold:
			data = Controller_Data_a.Public_Data.TempeThreshold[channel-1];;
			break;
		case Read_SlavesNumber:
			data = Controller_Data_a.Public_Data.SlavesNumb;
			break;
		default:
			break;
	}
	return data;
}

void Set_Controller_Prog_Data(unsigned char command,unsigned char recipe,unsigned char TriggerSource,unsigned int data)		
{
	recipe = recipe - 1;
	TriggerSource = TriggerSource - 1;
	switch (command)
	{
		case Set_Prog_Total_steps:
			Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Current_steps:
			Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Trigger_mode:
			Controller_Data_a.Programmable_Data.Prog_Trigger_mode[recipe][TriggerSource] = data;
			break;
		case Set_Prog_trigger_interval:
			Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Camera_delay:
			Controller_Data_a.Programmable_Data.Prog_Camera_delay[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Light_delay:
			Controller_Data_a.Programmable_Data.Prog_Light_delay[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Step_switch:
			Controller_Data_a.Programmable_Data.Prog_Step_switch[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Start_Steps:
			Controller_Data_a.Programmable_Data.Prog_Start_Steps[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Stop_Steps:
			Controller_Data_a.Programmable_Data.Prog_Stop_Steps[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Camera_output:
			//Controller_Data_a.Programmable_Data.Prog_Camera_output[recipe][TriggerSource] = data;
			break;
		case Set_Prog_Reset_steps:
			Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe][TriggerSource] = data;
			break;
		default:
			break;
	}
}

unsigned int Get_Controller_Prog_Data(unsigned char command,unsigned char recipe,unsigned char TriggerSource)
{
	int data = 0;
	recipe = recipe - 1;
	TriggerSource = TriggerSource - 1;
	switch (command)
	{
		case Read_Prog_Total_steps:
			data = Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource];
			break;
		case Read_Prog_Current_steps:
			data = Controller_Data_a.Programmable_Data.Prog_Current_steps[recipe][TriggerSource];
			break;
		case Read_Prog_Trigger_mode:
			data = Controller_Data_a.Programmable_Data.Prog_Trigger_mode[recipe][TriggerSource];
			break;
		case Read_Prog_trigger_interval:
			data = Controller_Data_a.Programmable_Data.Prog_trigger_interval[recipe][TriggerSource];
			break;
		case Read_Prog_Camera_delay:
			data = Controller_Data_a.Programmable_Data.Prog_Camera_delay[recipe][TriggerSource];
			break;
		case Read_Prog_Light_delay:
			data = Controller_Data_a.Programmable_Data.Prog_Light_delay[recipe][TriggerSource];
			break;
		case Read_Prog_Step_switch:
			data = Controller_Data_a.Programmable_Data.Prog_Step_switch[recipe][TriggerSource];
			break;
		case Read_Prog_Start_Steps:
			data = Controller_Data_a.Programmable_Data.Prog_Start_Steps[recipe][TriggerSource];
			break;
		case Read_Prog_Stop_Steps:
			data = Controller_Data_a.Programmable_Data.Prog_Stop_Steps[recipe][TriggerSource];
			break;
		case Read_Prog_Camera_output:
			//data = Controller_Data_a.Programmable_Data.Prog_Camera_output[recipe][TriggerSource];
			break;
		default:
			break;
	}
	return data;
}


void Set_Controller_Prog_TableData(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned char channel_number,unsigned int data)		
{
	int max = 255;

	if(controller_model[0] == Digital)
	{
		if(Controller_Data_a.Digital_data.brightness_level == 0)
		{
			max = brightness_MAX_1;
		}else
		{
			max = brightness_MAX_2;
		}
		if(data > max)
			data = max;
	}else if(controller_model[0] == Strobe)
	{
		if(data > Prog_value_MAX)
			data = Prog_value_MAX;
	}
	
	Controller_Data_a.Programmable_Data.Prog_data[recipe-1][TriggerSource-1][Line_number-1][channel_number] = data;
}
unsigned int Get_Controller_Prog_TableData(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned char channel_number)		
{
	return Controller_Data_a.Programmable_Data.Prog_data[recipe-1][TriggerSource-1][Line_number-1][channel_number];
}

void Set_Controller_Prog_TablePulseWidth(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned int data)		
{
	if(data > Prog_Pulse_width_MAX)
		data = Prog_Pulse_width_MAX;

	Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[recipe-1][TriggerSource-1][Line_number-1] = data;
}
void Set_Controller_Prog_TableCameraOutput(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number,unsigned char data)		
{
	Controller_Data_a.Programmable_Data.Prog_Camera_output[recipe-1][TriggerSource-1][Line_number-1] = data;
}

unsigned int Get_Controller_Prog_TablePulseWidth(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number)		
{
	return Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[recipe-1][TriggerSource-1][Line_number-1];
}

unsigned char Get_Controller_Prog_TableCameraOutput(unsigned char recipe,unsigned int TriggerSource,unsigned char Line_number)		
{
	return Controller_Data_a.Programmable_Data.Prog_Camera_output[recipe-1][TriggerSource-1][Line_number-1];
}


void Erasing_Programmable_Data(unsigned char recipe,unsigned int TriggerSource)		
{
	int Line = 0;
	int channel = 0;
	recipe = recipe-1;
	TriggerSource = TriggerSource-1;

	for(Line = 0; Line < Line_SUM; Line++)
	{
		for(channel = 0; channel < CHANNEL_NUM; channel++)
		{
			Controller_Data_a.Programmable_Data.Prog_data[recipe][TriggerSource][Line][channel] = 0;
		}
		Controller_Data_a.Programmable_Data.Prog_data_PulseWidth[recipe][TriggerSource][Line] = 0;
		Controller_Data_a.Programmable_Data.Prog_Camera_output[recipe][TriggerSource][Line] = 0;
	}
	Controller_Data_a.Programmable_Data.Prog_Total_steps[recipe][TriggerSource] = 0;
}




void read_version(void)
{
	unsigned char tx_len = 6;
	unsigned char tx_buffer[6] = {0};

	tx_buffer[0] = 0x72;
	tx_buffer[1] = 0x68;
	tx_buffer[2] = 0xbb;
	tx_buffer[3] = version[0];
	tx_buffer[4] = version[1];
	tx_buffer[5] = 0x16;
	package_tx_buffer(tx_buffer, tx_len);
}


unsigned int upguade_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	
	if((uart_buf[i+1] == 0x68)&&(uart_buf[i+2] == 0xaa)&&(uart_buf[i+5] == 0x16))  
	{
		if((uart_buf[i+3] == version[0])&&(uart_buf[i+4] == version[1]))
		{
			read_version();
			return i+5;
		}
		BootLoader_Reset();
		return 0;
	}
	if((uart_buf[i+1] == 0x68)&&(uart_buf[i+2] == 0xbb)&&(uart_buf[i+3] == 0x16))  
	{
		read_version();
		return i+3;
	}
	return i;
}


unsigned int BootLoader_cmd_handle_func(unsigned char *uart_buf,unsigned int i)
{

	switch (uart_buf[i])
	{
		case 0x72:
			i = upguade_cmdhandle(uart_buf,i);
			break;
	}
	return i;
}



unsigned int Transfer_cmd_handle_func(unsigned char *uart_buf,unsigned int i)
{
	unsigned char finish_flag = i;
	
	i = Old_cmd_handle_func(uart_buf,i);
	
	if(finish_flag != i)	//Explanation successful, the value of i will change
		return i;
	
	i = BootLoader_cmd_handle_func(uart_buf,i);
	if(finish_flag != i)
		return i;
	
	i = New_cmd_handle_func(uart_buf,i);

	return i;
}



