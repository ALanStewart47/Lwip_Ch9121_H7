#include "protocol_public.h"

typedef enum
{
	extend_Software_Version = 0,
	extend_Brightness,
	extend_Pulse_width,
	extend_Pulse_width_unit,
	extend_Working_Mode,
	extend_Channel_Switch,
	extend_Light_delay,
	extend_Camera_delay,
	extend_Digital_trigger_mode,
	extend_Trigger_cycle,
	extend_External_trigger_mode,
	extend_Camera_trigger_mode,
	extend_Channel_Number,
	extend_Color_temperature
} Extend_Command_Function;	

#define extend_Working_Mode_MAX  6

unsigned char Find_the_End(unsigned char *buffer)
{
	unsigned char channel_num = 0;

	for(channel_num = 0; channel_num < CHANNEL_NUM; channel_num++)
	{
		if((buffer[0 + channel_num*5]-'A' + 1) > CHANNEL_NUM)
		{
			return 0;		//Channel overflow
		}
		if(buffer[5 + channel_num*5] == '#')
		{
			return channel_num+1;
		}
	}
	return 0;		//not found end
}

unsigned char Find_Read_the_End(unsigned char *buffer)
{
	unsigned char channel_num = 0;

	for(channel_num = 0; channel_num < CHANNEL_NUM; channel_num++)
	{
		if((buffer[0 + channel_num*1]-'A' + 1) > CHANNEL_NUM)
		{
			return 0;		//Channel overflow
		}
		if(buffer[1 + channel_num*1] == '#')
		{
			return channel_num+1;
		}
	}
	return 0;		//not found end
}

unsigned char Command_Transfer_extend(unsigned char function)
{
	int command = 0;
	
	switch (function)
	{
		case extend_Brightness:
			command = Set_Brightness;
			break;
		case extend_Channel_Switch:
			command = Set_Channel_Switch;
			break;
		case extend_Digital_trigger_mode:
			command = Set_Digital_trigger_mode;
			break;
		case extend_Color_temperature:
			command = Set_Color_temperature;
			break;
		
		case extend_Pulse_width:
			command = Set_Pulse_width;
			break;
		case extend_Light_delay:
			command = Set_Light_delay;
			break;
		case extend_Camera_delay:
			command = Set_Camera_delay;
			break;
		case extend_Trigger_cycle:
			command = Set_Trigger_cycle;
			break;
		case extend_Working_Mode:
			command = Set_trigger_mode;
			break;
		case extend_Camera_trigger_mode:
			command = Set_Camera_trigger_mode;
			break;
		case extend_Pulse_width_unit:
			command = Set_Pulse_width_unit;
			break;
		//case extend_Trigger_filtering:
			//command = Set_Trigger_filtering;
			//break;
		default:
			break;
	}
	return command;
}

unsigned char Command_Transfer_Read_extend(unsigned char function)
{
	int command = 0;
	
	switch (function)
	{
		case extend_Brightness:
			command = Read_Brightness;
			break;
		case extend_Channel_Switch:
			command = Read_Channel_Switch;
			break;
		case extend_Digital_trigger_mode:
			command = Read_Digital_trigger_mode;
			break;
		case extend_Color_temperature:
			command = Read_Color_temperature;
			break;
		
		case extend_Pulse_width:
			command = Read_Pulse_width;
			break;
		case extend_Light_delay:
			command = Read_Light_delay;
			break;
		case extend_Camera_delay:
			command = Read_Camera_delay;
			break;
		case extend_Trigger_cycle:
			command = Read_Trigger_cycle;
			break;
		case extend_Working_Mode:
			command = Read_trigger_mode;
			break;
		case extend_Camera_trigger_mode:
			command = Read_Camera_trigger_mode;
			break;
		case extend_Pulse_width_unit:
			command = Read_Pulse_width_unit;
			break;
		//case extend_Trigger_filtering:
			//command = Set_Trigger_filtering;
			//break;
		default:
			break;
	}
	return command;
}


unsigned int Get_extend_Data_MAX(unsigned char function)
{
	int data = 0;
	
	switch (function)
	{
		case extend_Brightness:
			data = Get_Controller_Data_MAX(Set_Brightness);
			break;
		case extend_Channel_Switch:
			data = Channel_Switch_MAX;
			break;
		case extend_Digital_trigger_mode:
			data = Digital_trigger_mode_MAX;
			break;
		case extend_Color_temperature:
			data = Color_temperature_MAX;
			break;
		case extend_Pulse_width:
			data = Pulse_width_MAX;
			break;
		case extend_Light_delay:
			data = Light_delay_MAX;
			break;
		case extend_Camera_delay:
			data = Camera_delay_MAX;
			break;
		case extend_Trigger_cycle:
			data = Trigger_cycle_MAX;
			break;
		case extend_Working_Mode:
			data = extend_Working_Mode_MAX;
			break;
		case extend_Camera_trigger_mode:
			data = Camera_trigger_mode_MAX;
			break;
		case extend_Pulse_width_unit:
			data = Pulse_width_unit_MAX;
			break;
		default:
			break;
	}
	return data;
}

void Reply_write_Success(void)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = "$";

	package_tx_buffer(tx_buffer, tx_len);
}

void Set_extend_Data_cmd(unsigned char function,unsigned char *buffer,unsigned char data_sum)
{
	int channel_number = 0;
	unsigned char channel_no[CHANNEL_NUM] = {0};
	int data[CHANNEL_NUM] = {0};
	unsigned char command = Command_Transfer_extend(function);
	int Data_max = Get_Controller_Data_MAX(command);
	
	if(function == extend_Working_Mode)			//This feature has a maximum of 6
		Data_max = extend_Working_Mode_MAX;
	
	for(channel_number = 0; channel_number < data_sum; channel_number++)
	{
		channel_no[channel_number] = buffer[0 + 5*channel_number]-'A'+1;
		data[channel_number] = (buffer[2+ 5*channel_number]-'0')*100 
			+ (buffer[3 + 5*channel_number]-'0')*10 + (buffer[4 + 5*channel_number]-'0');
		if(data[channel_number] > Data_max)			//data overflow
		{
			return ;
		}
	}

	for(channel_number = 0; channel_number < data_sum; channel_number++)
	{
		if(function == extend_Working_Mode)
		{
			if(data[channel_number] == 1)		//0001
			{
				data[channel_number] = 0;
			}else if(data[channel_number] == 4)		//0004
			{
				data[channel_number] = 1;
			}else if(data[channel_number] == 6)		//0006
			{
				data[channel_number] = 2;
			}else
			{
				return;
			}
		}
		Set_Controller_Data(command,channel_no[channel_number],data[channel_number]);
	}
	Reply_write_Success();
	return ;
}

unsigned int Extend_write_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char function = (uart_buf[i+1]-'0')*10 + (uart_buf[i+2]-'0');
	unsigned char *channel_position = &uart_buf[i+3];
	unsigned char channel_num = 0;
	unsigned char end = (uart_buf[i+8]);

	if((function >= extend_Brightness)&&(function <= extend_Color_temperature))
	{
	}else
	{
		return i;
	}
	switch (function)
	{
		case extend_Working_Mode:
		case extend_Digital_trigger_mode:
		case extend_Trigger_cycle:		//Invalid channel symbol,fixed length
			if(end != '#')
				return i;
			Set_extend_Data_cmd(function,channel_position,1);
			return i + 8;
		default:					//Channel symbol is valid,Indefinite length
			channel_num = Find_the_End(channel_position);
			if(channel_num == 0)
				return i;
			Set_extend_Data_cmd(function,channel_position,channel_num);
			return i + 3 + channel_num*5;
	}
}

void Read_extend_Data_cmd(unsigned char function,unsigned char *buffer,unsigned char data_sum)
{
	int channel_number = 0;
	unsigned char channel_no = 0;
	int data = 0;
	unsigned char command = Command_Transfer_Read_extend(function);
	unsigned int tx_len = 1;
	unsigned char tx_buffer[SUM_SIZE] = "@";

	tx_buffer[tx_len++] = function/10 + '0';
	tx_buffer[tx_len++] = function%10 + '0';

	for(channel_number = 0; channel_number < data_sum; channel_number++)
	{
		tx_buffer[tx_len++] = buffer[0 + channel_number];
		channel_no = buffer[0 + channel_number]-'A'+1;
		data = Get_Controller_Data(command,channel_no);
		if(function == extend_Working_Mode)
		{
			if(data == 0)		//0001
			{
				data = 1;
			}else if(data == 1)		//0004
			{
				data = 4;
			}else if(data == 2)		//0006
			{
				data = 6;
			}else
			{
				return;
			}
		}
		tx_buffer[tx_len++] = data / 1000 + '0';
		tx_buffer[tx_len++] = data % 1000 /100 + '0';
		tx_buffer[tx_len++] = data % 100 /10 + '0';
		tx_buffer[tx_len++] = data % 10 + '0';
	}
	tx_buffer[tx_len++] = '#';
	package_tx_buffer(tx_buffer, tx_len);
	return ;
}

unsigned int Extend_read_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char function = (uart_buf[i+1]-'0')*10 + (uart_buf[i+2]-'0');
	unsigned char *channel_position = &uart_buf[i+3];
	unsigned char channel_num = 0;
	unsigned char end = (uart_buf[i+4]);

	if((function >= extend_Brightness)&&(function <= extend_Color_temperature))
	{
	}else
	{
		return i;
	}
	switch (function)
	{
		case extend_Working_Mode:
		case extend_Digital_trigger_mode:
		case extend_Trigger_cycle:		//Invalid channel symbol,fixed length
			if(end != '#')
				return i;
			Read_extend_Data_cmd(function,channel_position,1);
			return i+4;
		default:					//Channel symbol is valid,Indefinite length
			channel_num = Find_Read_the_End(channel_position);
			if(channel_num == 0)
				return i;
			Read_extend_Data_cmd(function,channel_position,channel_num);
			return i + 3 + channel_num*1;
	}
}



