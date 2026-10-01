#include "protocol_public.h"

#define    CRC_ENABLE		1

#define    Single_Channel_Head		0xCA
#define    Multi_Channel_Head		0xCB
#define    Programmable_Head		0xCC

typedef enum
{
	success = 0,
	Command_Not_Supported,
	Channel_Overflow,
	Data_Overflow
} ErrorCode;		


typedef struct Channel_Data{
	unsigned char channel_number;
	unsigned int channel_value;
}Channel_Data;


//BCC checksum calculation function buf: data starting address len: length return: checksum
unsigned char BccChecksum(unsigned char* buf,int len)
{
	int i = 0;
	unsigned char checksum = 0;
	
	for(i = 0;i < len; i++)
	{
		checksum ^= buf[i];
	}
	
	return checksum;
}
void Reply_Fail_Command(unsigned char head,unsigned char command,unsigned char channel,unsigned char code)
{
	unsigned char tx_len = 4;
	unsigned char tx_buffer[4] = {0};

	tx_buffer[0] = head;
	tx_buffer[1] = command;		
	tx_buffer[2] = channel;		
	tx_buffer[3] = code;

	package_tx_buffer(tx_buffer, tx_len);
}

void Reply_Not_Supported(unsigned char head,unsigned char command,unsigned char channel)
{
	Reply_Fail_Command(head,command,channel,Command_Not_Supported);
}

void Reply_ChannelFail_Command(unsigned char head,unsigned char command,unsigned char channel)
{
	Reply_Fail_Command(head,command,channel,Channel_Overflow);
}

void Reply_DataFail_Command(unsigned char head,unsigned char command,unsigned char channel)
{
	Reply_Fail_Command(head,command,channel,Data_Overflow);
}

void Reply_Set_Success_Command(unsigned char head,unsigned char command,unsigned char channel)
{
	unsigned char tx_len = 4;
	unsigned char tx_buffer[4] = {0};

	tx_buffer[0] = head;
	tx_buffer[1] = command;		
	tx_buffer[2] = channel;	
	tx_buffer[3] = success;

	package_tx_buffer(tx_buffer, tx_len);
}


void Reply_Read_Success_Command(unsigned char head,unsigned char command,unsigned char channel)
{
	unsigned char tx_len = 6;
	unsigned char tx_buffer[6] = {0};
	unsigned char crc = 0;
	unsigned int data = Get_Controller_Data(command,channel);
	
	tx_buffer[0] = head;
	tx_buffer[1] = command;		
	tx_buffer[2] = channel;	
	tx_buffer[3] = data / 256;
	tx_buffer[4] = data % 256;
	crc = BccChecksum(tx_buffer,5);
	tx_buffer[5] = crc;	

	package_tx_buffer(tx_buffer, tx_len);
}



void Set_DATA_NEWcmd(unsigned char head,unsigned char Command,Channel_Data *data,unsigned char data_sum)
{
	int channel_number = 0;
	int Data_max = Get_Controller_Data_MAX(Command);
	
	for(channel_number = 0; channel_number < data_sum; channel_number++)
	{
		if(data[channel_number].channel_number> CHANNEL_NUM)
		{
			Reply_ChannelFail_Command(head,Command,data[channel_number].channel_number);
			return ;
		}

		if(data[channel_number].channel_value > Data_max)
		{
			Reply_DataFail_Command(head,Command,data[channel_number].channel_number);
			return ;
		}
		Set_Controller_Data(Command,data[channel_number].channel_number,data[channel_number].channel_value);
	}

	if(data_sum == 1)		//1  Single channel successful reply
	{
		Reply_Set_Success_Command(head,Command,data[0].channel_number);	
	}else
	{
		Reply_Set_Success_Command(head,Command,0);		//Multi channel 0
	}
	return ;
}

void Read_Data_NEWcmd(unsigned char head,unsigned char Command,unsigned char channel)
{
	if(channel> CHANNEL_NUM)
	{
		Reply_ChannelFail_Command(head,Command,channel);
		return ;
	}

	Reply_Read_Success_Command(head,Command,channel);
	return ;
}

unsigned int SingleChannel_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char head = uart_buf[i];
	unsigned char Command = uart_buf[i+1];
	unsigned char channel = uart_buf[i+2];
	unsigned int value = uart_buf[i+3] *256 + uart_buf[i+4];
	Channel_Data data;
	unsigned char crc = BccChecksum(&uart_buf[i],5);

	if(crc != uart_buf[i+5])
	{
		if(CRC_ENABLE == 1)
		{
			return i;
		}
	}
	data.channel_number = channel;
	data.channel_value = value;
	switch (Command)
	{
		case Set_Brightness:
		case Set_Channel_Switch:
		case Set_Digital_trigger_mode:
		case Set_Color_temperature:
		case Set_brightness_level:
			Set_DATA_NEWcmd(head,Command,&data,1);
			break;
		case Read_Brightness:
		case Read_Channel_Switch:
		case Read_Digital_trigger_mode:
		case Read_Color_temperature:
		case Read_brightness_level:
			Read_Data_NEWcmd(head,Command,channel);
			break;
		case Set_Pulse_width:
		case Set_Light_delay:
		case Set_Camera_delay:
		case Set_Trigger_cycle:
		case Set_trigger_mode:
		case Set_Camera_trigger_mode:
		case Set_Pulse_width_unit:
		case Set_Trigger_filtering:
			Set_DATA_NEWcmd(head,Command,&data,1);
			break;
		case Read_Pulse_width:
		case Read_Light_delay:
		case Read_Camera_delay:
		case Read_Trigger_cycle:
		case Read_trigger_mode:
		case Read_Camera_trigger_mode:
		case Read_Pulse_width_unit:
		case Read_Trigger_filtering:
			Read_Data_NEWcmd(head,Command,channel);
			break;
		case Set_mode:
		case Set_Baud_rate:
		case Set_Clean_Input_Output_TrigNumber:
		case Set_softwareTrig:
		case Set_RestoreFactorySettings:
		case Set_DataSave:
		case Set_TemperatureThreshold:
		case Set_ExploreSlave:
		case Set_SynMode:
			Set_DATA_NEWcmd(head,Command,&data,1);
			break;
		case Read_mode:
		case Read_Baud_rate:
		case Read_Input_triggers_number:
		case Read_LightOutput_triggers_number:
		case Read_CameraOutput_triggers_number:
		case Read_Number_of_channels:
		case Read_Controller_model:
		case Read_TemperatureThreshold:
		case Read_SlavesNumber:
		case Read_SynMode:
			Read_Data_NEWcmd(head,Command,channel);
			break;
		default:
			Reply_Not_Supported(head,Command,channel);	//Return unsupported command
			break;
	}
	
	return i+5;
}

void transform_channelData(Channel_Data *data1_out,unsigned char *data1_in,unsigned char data_sum)
{
	int i = 0;
	for(i = 0; i < data_sum; i++)
	{
		data1_out[i].channel_number = data1_in[0 + i*3];
		data1_out[i].channel_value =  data1_in[1 + i*3] *256 + data1_in[2 + i*3];
	}
}

unsigned int MultiChannel_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char head = uart_buf[i];
	unsigned char Command = uart_buf[i+1];
	unsigned char channel_counts = uart_buf[i+2];
	unsigned int length = 3 + channel_counts*3;
	Channel_Data data [channel_counts];
	unsigned char crc = BccChecksum(&uart_buf[i],length);

	if(crc != uart_buf[i + 3 + channel_counts*3])
	{
		if(CRC_ENABLE == 1)
		{
			return i;
		}
	}
	transform_channelData(data,&uart_buf[i+3],channel_counts);
	switch (Command)
	{
		case Set_Brightness:
		case Set_Channel_Switch:
		case Set_Color_temperature:
		case Set_brightness_level:
			Set_DATA_NEWcmd(head,Command,data,channel_counts);
			break;
		case Set_Pulse_width:
		case Set_Light_delay:
		case Set_Camera_delay:
			Set_DATA_NEWcmd(head,Command,data,channel_counts);
			break;
		default:
			Reply_Not_Supported(head,Command,channel_counts);	//Return unsupported command
			break;
	}
	
	return i+length;
}

unsigned int Set_ProgTable_data_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char head = uart_buf[i];
	unsigned char Command = uart_buf[i+1];
	unsigned char recipe = uart_buf[i+2];
	unsigned char TriggerSource = uart_buf[i+3];
	unsigned int Line_number = uart_buf[i+4] *256 + uart_buf[i+5];
	unsigned int channel_count = uart_buf[i+6] *256 + uart_buf[i+7];
	unsigned char Camera_output = uart_buf[i+8];
	unsigned int Pulse_width = uart_buf[i+10] *256 + uart_buf[i+11];
	unsigned int length = 12 + channel_count*2;
	unsigned char crc = BccChecksum(&uart_buf[i],length);
	int channel_number = 0;
	int value = 0;

	if(crc != uart_buf[i +12 + channel_count*2])
	{
		if(CRC_ENABLE == 1)
		{
			return i;
		}
	}
	if(recipe > RECIPE_SUM)
	{
		Reply_DataFail_Command(head,Command,1);
		return i+length;
	}
	if(TriggerSource > TriggerSource_SUM)
	{
		Reply_DataFail_Command(head,Command,2);
		return i+length;
	}
	if(Line_number > Line_SUM)
	{
		Reply_DataFail_Command(head,Command,3);
		return i+length;
	}
	if(channel_count > CHANNEL_NUM)			//channel over
	{
		Reply_DataFail_Command(head,Command,4);
		return i+length;
	}

	Set_Controller_Prog_TablePulseWidth(recipe,TriggerSource,Line_number,Pulse_width);
	Set_Controller_Prog_TableCameraOutput(recipe,TriggerSource,Line_number,Camera_output);
	for(channel_number = 0; channel_number < channel_count; channel_number++)
	{
		value = uart_buf[12 + channel_number*2] *256 + uart_buf[13 + channel_number*2];
		Set_Controller_Prog_TableData(recipe,TriggerSource,Line_number,channel_number,value);
	}
	Set_Controller_ProgData_Callback(Command,recipe,TriggerSource,Line_number);
	Reply_Set_Success_Command(head,Command,0);
	return i+length;
}
void Repl_Read_Success_Command(unsigned char head,unsigned char command,unsigned char channel)
{
	unsigned char tx_len = 6;
	unsigned char tx_buffer[6] = {0};
	unsigned char crc = 0;
	unsigned int data = Get_Controller_Data(command,channel);
	
	tx_buffer[0] = head;
	tx_buffer[1] = command;		
	tx_buffer[2] = channel;	
	tx_buffer[3] = data / 256;
	tx_buffer[4] = data % 256;
	crc = BccChecksum(tx_buffer,5);
	tx_buffer[5] = crc;	

	package_tx_buffer(tx_buffer, tx_len);
}

unsigned int Read_ProgTable_data_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char head = uart_buf[i];
	unsigned char Command = uart_buf[i+1];
	unsigned char recipe = uart_buf[i+2];
	unsigned char TriggerSource = uart_buf[i+3];
	unsigned int Line_number = uart_buf[i+4] *256 + uart_buf[i+5];
	unsigned int channel_count = uart_buf[i+6] *256 + uart_buf[i+7];
	unsigned char Camera_output = 0;
	unsigned int Pulse_width = 0;
	unsigned char crc = BccChecksum(&uart_buf[i],8);
	int channel_number = 0;
	int value = 0;
	unsigned int tx_len = 12 + channel_count*2 +1;
	unsigned char tx_buffer[SUM_SIZE] = {0};

	Get_Controller_ProgData_Callback(Command,recipe,TriggerSource,Line_number);

	if(crc != uart_buf[i +8])
	{
		if(CRC_ENABLE == 1)
		{
			return i;
		}
	}
	if(recipe > RECIPE_SUM)
	{
		Reply_DataFail_Command(head,Command,1);
		return i+8;
	}
	if(TriggerSource > TriggerSource_SUM)
	{
		Reply_DataFail_Command(head,Command,2);
		return i+8;
	}
	if(Line_number > Line_SUM)
	{
		Reply_DataFail_Command(head,Command,3);
		return i+8;
	}
	if(channel_count > CHANNEL_NUM)			//channel over
	{
		Reply_DataFail_Command(head,Command,4);
		return i+8;
	}
	
	Pulse_width = Get_Controller_Prog_TablePulseWidth(recipe,TriggerSource,Line_number);
	Camera_output = Get_Controller_Prog_TableCameraOutput(recipe,TriggerSource,Line_number);
	tx_buffer[0] = head;
	tx_buffer[1] = Command;		
	tx_buffer[2] = recipe;
	tx_buffer[3] = TriggerSource;
	tx_buffer[4] = Line_number / 256;
	tx_buffer[5] = Line_number % 256;
	tx_buffer[6] = channel_count / 256;
	tx_buffer[7] = channel_count % 256;
	tx_buffer[8] = Camera_output;
	tx_buffer[9] = 0;
	tx_buffer[10] = Pulse_width / 256;
	tx_buffer[11] = Pulse_width % 256;
	for(channel_number = 0; channel_number < channel_count; channel_number++)
	{
		value = Get_Controller_Prog_TableData(recipe,TriggerSource,Line_number,channel_number);
		tx_buffer[12 + channel_number*2] = value / 256;
		tx_buffer[13 + channel_number*2] = value % 256;
	}
	crc = BccChecksum(tx_buffer,12 + channel_count*2);
	tx_buffer[12 + channel_count*2] = crc;	

	package_tx_buffer(tx_buffer, tx_len);
	//Reply_Set_Success_Command(head,Command,0);
	return i+8;
}

void Set_ProgDATA_NEWcmd(unsigned char head,unsigned char Command,unsigned char recipe,unsigned char TriggerSource,unsigned int data)
{
	int Data_max = Get_Controller_Prog_Data_MAX(Command);
	
	if(recipe > RECIPE_SUM)
	{
		Reply_DataFail_Command(head,Command,1);
		return ;
	}
	if(TriggerSource > TriggerSource_SUM)
	{
		Reply_DataFail_Command(head,Command,2);
		return ;
	}
	if(data > Data_max)
	{
		Reply_DataFail_Command(head,Command,4);
		return ;
	}
	
	Set_Controller_Prog_Data(Command,recipe,TriggerSource,data);
	Set_Controller_ProgData_Callback(Command,recipe,TriggerSource,0);

	Reply_Set_Success_Command(head,Command,0);
	return ;
}

void Reply_Prog_Read_Success_Command(unsigned char head,unsigned char command,unsigned char recipe,unsigned char TriggerSource)
{
	unsigned char tx_len = 7;
	unsigned char tx_buffer[7] = {0};
	unsigned char crc = 0;
	unsigned int data = Get_Controller_Prog_Data(command,recipe,TriggerSource);
	
	tx_buffer[0] = head;
	tx_buffer[1] = command;		
	tx_buffer[2] = recipe;	
	tx_buffer[3] = TriggerSource;	
	tx_buffer[4] = data / 256;
	tx_buffer[5] = data % 256;
	crc = BccChecksum(tx_buffer,6);
	tx_buffer[6] = crc;	

	package_tx_buffer(tx_buffer, tx_len);
}

void Read_ProgData_NEWcmd(unsigned char head,unsigned char Command,unsigned char recipe,unsigned char TriggerSource)
{
	if(recipe > RECIPE_SUM)
	{
		Reply_DataFail_Command(head,Command,1);
		return ;
	}
	if(TriggerSource > TriggerSource_SUM)
	{
		Reply_DataFail_Command(head,Command,2);
		return ;
	}
	Set_Controller_ProgData_Callback(Command,recipe,TriggerSource,0);
	Reply_Prog_Read_Success_Command(head,Command,recipe,TriggerSource);
	return ;
}
void Set_Erasing_Programmable_Data(unsigned char head,unsigned char Command,unsigned char recipe,unsigned char TriggerSource)
{
	if(recipe > RECIPE_SUM)
	{
		Reply_DataFail_Command(head,Command,1);
		return ;
	}
	if(TriggerSource > TriggerSource_SUM)
	{
		Reply_DataFail_Command(head,Command,2);
		return ;
	}
	
	Erasing_Programmable_Data(recipe,TriggerSource);
	Set_Controller_ProgData_Callback(Command,recipe,TriggerSource,0);

	Reply_Set_Success_Command(head,Command,0);
	return ;
}

void Set_Software_Trigger(unsigned char head,unsigned char Command,unsigned char recipe,unsigned char TriggerSource)
{
	if(recipe > RECIPE_SUM)
	{
		Reply_DataFail_Command(head,Command,1);
		return ;
	}
	if(TriggerSource > TriggerSource_SUM)
	{
		Reply_DataFail_Command(head,Command,2);
		return ;
	}

	Set_Controller_ProgData_Callback(Command,recipe,TriggerSource,0);
	//Software_Trigger(recipe,TriggerSource);

	Reply_Set_Success_Command(head,Command,0);
	return ;
}


unsigned int Programmable_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	unsigned char head = uart_buf[i];
	unsigned char Command = uart_buf[i+1];
	unsigned char recipe = uart_buf[i+2];
	unsigned char TriggerSource = uart_buf[i+3];
	unsigned int value = uart_buf[i+4] *256 + uart_buf[i+5];
	unsigned char crc = 0;

	if((Command != Set_Programmable_data)&&(Command != Read_Programmable_data))
	{
		crc = BccChecksum(&uart_buf[i],6);
		if(crc != uart_buf[i+6])
		{
			if(CRC_ENABLE == 1)
			{
				return i;
			}
		}
	}

	switch (Command)
	{
		case Set_Programmable_data:
			i = Set_ProgTable_data_cmdhandle(uart_buf,i);
			break;
		case Read_Programmable_data:
			i = Read_ProgTable_data_cmdhandle(uart_buf,i);
			break;
		case Set_Prog_Total_steps:
		case Set_Prog_Current_steps:
		case Set_Prog_Trigger_mode:
		case Set_Prog_trigger_interval:
		case Set_Prog_Camera_delay:
		case Set_Prog_Light_delay:
		case Set_Prog_Step_switch:
		case Set_Prog_Start_Steps:
		case Set_Prog_Stop_Steps:
		case Set_Prog_Camera_output:
		case Set_Prog_HardwareResetSwitch:
		case Set_Prog_AutoResetSwitch:
		case Set_Prog_ResetTime:
			Set_ProgDATA_NEWcmd(head,Command,recipe,TriggerSource,value);
			break;
		case Read_Prog_Total_steps:
		case Read_Prog_Current_steps:
		case Read_Prog_Trigger_mode:
		case Read_Prog_trigger_interval:
		case Read_Prog_Camera_delay:
		case Read_Prog_Light_delay:
		case Read_Prog_Step_switch:
		case Read_Prog_Start_Steps:
		case Read_Prog_Stop_Steps:
		case Read_Prog_Camera_output:
		case Read_Prog_HardwareResetSwitch:
		case Read_Prog_AutoResetSwitch:
		case Read_Prog_ResetTime:
			Read_ProgData_NEWcmd(head,Command,recipe,TriggerSource);
			break;
		case Set_Prog_Reset_steps:
			Set_ProgDATA_NEWcmd(head,Command,recipe,TriggerSource,1);
			break;
		case Set_Prog_Erase_data:
			Set_Erasing_Programmable_Data(head,Command,recipe,TriggerSource);
			break;
		case Set_Prog_software_trigger:
			Set_Software_Trigger(head,Command,recipe,TriggerSource);
			break;
		default:
			Reply_Not_Supported(head,Command,0);	//Return unsupported command
			break;
	}

	if((Command == Set_Programmable_data)||(Command == Read_Programmable_data))
	{
		return i;
	}else
	{
		return i+6;
	}
}


unsigned int New_cmd_handle_func(unsigned char *uart_buf,unsigned int i)
{
	int head = uart_buf[i];		//Functional Head
	
	switch (head)
	{
		case Single_Channel_Head:
			i = SingleChannel_cmdhandle(uart_buf,i);
			break;
		case Multi_Channel_Head:
			i = MultiChannel_cmdhandle(uart_buf,i);
			break;
		case Programmable_Head:
			i = Programmable_cmdhandle(uart_buf,i);
			break;
	}
	return i;
}



