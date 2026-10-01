#include "protocol_public.h"

void set_brightness_API(unsigned char channel,unsigned int value)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};

	tx_buffer[0] = 'a' + channel;
	Set_Controller_Data(Set_Brightness,channel+1,value);

	package_tx_buffer(tx_buffer, tx_len);
}
void read_brightness_API(unsigned char channel)
{
	unsigned char tx_len = 5;
	unsigned char tx_buffer[5] = {0};
	unsigned int temp = Get_Controller_Data(Read_Brightness,channel+1);

	tx_buffer[0] = 'a' + channel;
	tx_buffer[1] = '0';
	tx_buffer[2] = temp/100 + '0';
	tx_buffer[3] = temp%100/10 + '0';
	tx_buffer[4] = temp%10 + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}

void set_trigger_cycle_API(unsigned int value)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};

	tx_buffer[0] = 't' ;
	Set_Controller_Data(Set_Trigger_cycle,1,value);

	package_tx_buffer(tx_buffer, tx_len);
}

void read_trigger_cycle_API(void)
{
	unsigned char tx_len = 5;
	unsigned char tx_buffer[5] = {0};
	unsigned int temp = Get_Controller_Data(Read_Trigger_cycle,1);

	tx_buffer[0] = 't';
	tx_buffer[1] = '0';
	tx_buffer[2] = temp/100 + '0';
	tx_buffer[3] = temp%100/10 + '0';
	tx_buffer[4] = temp%10 + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}
void set_area_brightness_API(unsigned char channel)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};
	
	//对外接口位置
	tx_buffer[0] = 'a'+ channel ;

	package_tx_buffer(tx_buffer, tx_len);
}
void read_area_brightness_API(void)
{
	unsigned char tx_len = 6;
	unsigned char tx_buffer[6] = {0};
	unsigned int temp = 0;

	//对外接口位置
	tx_buffer[0] = 's';
	tx_buffer[1] = 'w';
	tx_buffer[2] = temp/100 + '0';
	tx_buffer[3] = temp%100/10 + '0';
	tx_buffer[4] = temp%10 + '0';
	tx_buffer[5] = temp%10 + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}

void soft_trigger_API(void)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};

	//软触发
	tx_buffer[0] = '#';
	package_tx_buffer(tx_buffer, tx_len);
}

void save_para_API(void)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};

	//保存参数
	tx_buffer[0] = '#';
	package_tx_buffer(tx_buffer, tx_len);
}

void read_SP_pulseWidth_unit_API(unsigned char channel)
{
	unsigned char tx_len = 4;
	unsigned char tx_buffer[4] = {0};
	unsigned int temp = Get_Controller_Data(Read_Pulse_width_unit,channel+1);

	tx_buffer[0] = 'p';
	tx_buffer[1] = 'u';
	tx_buffer[2] = channel + 'a';
	tx_buffer[3] = temp + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}

void set_SP_pulseWidth_unit_API(unsigned char channel,unsigned int value)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = {0};

	tx_buffer[0] = 'p';
	tx_buffer[1] = 'u';
	tx_buffer[2] = channel + 'a';
	Set_Controller_Data(Set_Pulse_width_unit,channel+1,value);

	package_tx_buffer(tx_buffer, tx_len);
}


void read_SP_pulseWidth_API(unsigned char channel)
{
	unsigned char tx_len = 6;
	unsigned char tx_buffer[6] = {0};
	unsigned int temp = Get_Controller_Data(Read_Pulse_width,channel+1);

	tx_buffer[0] = 'p';
	tx_buffer[1] = channel + 'a';
	tx_buffer[2] = '0';
	tx_buffer[3] = temp/100 + '0';
	tx_buffer[4] = temp%100/10 + '0';
	tx_buffer[5] = temp%10 + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}

void set_SP_pulseWidth_API(unsigned char channel,unsigned int value)
{
	unsigned char tx_len = 2;
	unsigned char tx_buffer[2] = {0};

	tx_buffer[0] = 'p';
	tx_buffer[1] =  channel + 'a';
	Set_Controller_Data(Set_Pulse_width,channel+1,value);

	package_tx_buffer(tx_buffer, tx_len);
}

void read_trigger_mode_API(void)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = {0};
	unsigned int temp = Get_Controller_Data(Read_trigger_mode,1);

	tx_buffer[0] = 't';
	tx_buffer[1] = 'r';
	tx_buffer[2] = '0'+ temp;
	package_tx_buffer(tx_buffer, tx_len);
}

void set_trigger_mode_API(unsigned int value)
{
	unsigned char tx_len = 2;
	unsigned char tx_buffer[2] = {0};

	tx_buffer[0] = 't';
	tx_buffer[1] = 'r';
	Set_Controller_Data(Set_trigger_mode,1,value);
	
	package_tx_buffer(tx_buffer, tx_len);
}

void set_horl_mode_API(unsigned int value)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};

	tx_buffer[0] = value-'A'+'a';
	if(tx_buffer[0] == 'h')
	{
		Set_Controller_Data(Set_Digital_trigger_mode,1,1);
	}else if(tx_buffer[0] == 'l')
	{
		Set_Controller_Data(Set_Digital_trigger_mode,1,0);
	}
	package_tx_buffer(tx_buffer, tx_len);
}

void read_horl_mode_API(void)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};
	unsigned int temp = Get_Controller_Data(Read_Digital_trigger_mode,1);

	if(temp == 1)
	{
		tx_buffer[0] = 'H';
	}else
	{
		tx_buffer[0] = 'L';
	}
	package_tx_buffer(tx_buffer, tx_len);
}

void set_triggerpower_API(unsigned int value) //设置上下升沿
{
	unsigned char tx_len = 2;
	unsigned char tx_buffer[2] = {0};

	tx_buffer[0] = 't';
	tx_buffer[1] = 'p';
	package_tx_buffer(tx_buffer, tx_len);
}

void read_triggerpower_API(void)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = "tp0";


	package_tx_buffer(tx_buffer, tx_len);
}

void set_light_delay_API(unsigned char channel,unsigned int value)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = {0};

	tx_buffer[0] = 'd';
	tx_buffer[1] = 'l';
	tx_buffer[2] =  channel + 'a';
	Set_Controller_Data(Set_Light_delay,channel+1,value);

	package_tx_buffer(tx_buffer, tx_len);
}

void set_camera_delay_API(unsigned char channel,unsigned int value)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = {0};

	tx_buffer[0] = 'd';
	tx_buffer[1] = 'c';
	tx_buffer[2] =	channel + 'a';
	Set_Controller_Data(Set_Camera_delay,channel+1,value);

	package_tx_buffer(tx_buffer, tx_len);
}

void set_AllCamera_delay_API(unsigned int value)
{
	unsigned char tx_len = 1;
	unsigned char tx_buffer[1] = {0};

	//对外接口位置
	tx_buffer[0] = '#';

	package_tx_buffer(tx_buffer, tx_len);
}

void read_light_delay_API(unsigned char channel)
{
	unsigned char tx_len = 7;
	unsigned char tx_buffer[7] = {0};
	unsigned int temp = Get_Controller_Data(Read_Light_delay,channel+1);

	tx_buffer[0] = 'd';
	tx_buffer[1] = 'l';
	tx_buffer[2] = channel + 'a';
	tx_buffer[3] = '0';
	tx_buffer[4] = temp/100 + '0';
	tx_buffer[5] = temp%100/10 + '0';
	tx_buffer[6] = temp%10 + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}

void read_camera_delay_API(unsigned char channel)
{
	unsigned char tx_len = 7;
	unsigned char tx_buffer[7] = {0};
	unsigned int temp = Get_Controller_Data(Read_Camera_delay,channel+1);

	tx_buffer[0] = 'd';
	tx_buffer[1] = 'c';
	tx_buffer[2] = channel + 'a';
	tx_buffer[3] = '0';
	tx_buffer[4] = temp/100 + '0';
	tx_buffer[5] = temp%100/10 + '0';
	tx_buffer[6] = temp%10 + '0';
	
	package_tx_buffer(tx_buffer, tx_len);
}

void read_AllCamera_delay_API(void)
{
	unsigned char tx_len = 6;
	unsigned char tx_buffer[6] = "dc0000";

	
	package_tx_buffer(tx_buffer, tx_len);
}

void set_trigger_UpRoDpwm_API(unsigned int value)
{
	unsigned char tx_len = 2;
	unsigned char tx_buffer[2] = {0};

	tx_buffer[0] = 'c';
	tx_buffer[1] = 't';
	Set_Controller_Data(Set_Camera_trigger_mode,1,value);

	package_tx_buffer(tx_buffer, tx_len);
}

void read_trigger_UpRoDpwm_API(void)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = {0};
	unsigned int temp = Get_Controller_Data(Read_Camera_trigger_mode,1);

	tx_buffer[0] = 'c';
	tx_buffer[1] = 't';
	tx_buffer[2] = '0' + temp;

	package_tx_buffer(tx_buffer, tx_len);
}

void retun_CST_API(void)
{
	unsigned char tx_len = 3;
	unsigned char tx_buffer[3] = "CST";


	package_tx_buffer(tx_buffer, tx_len);
}


unsigned char SX0XXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	unsigned int brightness = 0;
	int Data_max = Get_Controller_Data_MAX(Set_Brightness);
	if (uart_buf[i+2] == '0')
	{
		brightness = (uart_buf[i+3]- '0')*100+(uart_buf[i+4]- '0')*10+(uart_buf[i+5]- '0');
		if(brightness <= Data_max)
		{
			set_brightness_API(uart_buf[i+1]-'A',brightness);//设置亮度
			return i+6;
		}
	}
	return i;
}
unsigned char SX_CMD(unsigned char *uart_buf,unsigned int i)
{	
	read_brightness_API(uart_buf[i+1]-'A');//读亮度
	return i+2;		
}

unsigned int ST0XXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	unsigned int trigger_cycle  = 0;

	if(uart_buf[i+2] == '0')
	{
		trigger_cycle= (uart_buf[i+3]- '0')*100+(uart_buf[i+4]- '0')*10+(uart_buf[i+5]- '0');
		if(trigger_cycle <= Trigger_cycle_MAX)
		{
			set_trigger_cycle_API(trigger_cycle);//设置触发周期
			return i+6;
		}
	}
	return i;
}
unsigned int ST_CMD(unsigned char *uart_buf,unsigned int i)
{
	read_trigger_cycle_API();//读触发周期
	return i+2;
}

unsigned int SWXXXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	if(((uart_buf[i+2] >= '0')&&(uart_buf[i+2] <= '9'))&&((uart_buf[i+3] >= '0')&&(uart_buf[i+3] <= '9'))&&
		((uart_buf[i+4] >= '0')&&(uart_buf[i+4] <= '9'))&&((uart_buf[i+5] >= '0')&&(uart_buf[i+5] <= '9')))
	{
		set_area_brightness_API(uart_buf[i+1]-'A');//设置分区亮灭状态
		return i+6;
	}
	return i;
}
unsigned int SW_CMD(unsigned char *uart_buf,unsigned int i)
{
	read_area_brightness_API();//读取分区亮灭状态
	return i+2;
}

unsigned int SWTRIG_CMD(unsigned char *uart_buf,unsigned int i)
{
	
	if((uart_buf[i+2] == 'T')&&(uart_buf[i+3] == 'R')&&(uart_buf[i+4] == 'I')&&(uart_buf[i+5] == 'G'))
	{
		soft_trigger_API();
		return i+6;
	}
	return i;
}

unsigned int SAVE_CMD(unsigned char *uart_buf,unsigned int i)
{
	
	if((uart_buf[i+2] == 'V')&&(uart_buf[i+3] == 'E'))
	{
		save_para_API();
		return i+4;
	}
	return i;
}

unsigned int SPUX_CMD(unsigned char *uart_buf,unsigned int i)
{
	read_SP_pulseWidth_unit_API(uart_buf[i+3]-'A');
	return i+4;
}

unsigned int SXXX_CMD(unsigned char *uart_buf,unsigned int i)
{	
	//可编程模式参数读回
	return i+4;		
}

unsigned int SPX_CMD(unsigned char *uart_buf,unsigned int i)
{
	read_SP_pulseWidth_API(uart_buf[i+2]-'A');
	return i+3;
}

unsigned int SPUXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	if((uart_buf[i+4] == '0')||(uart_buf[i+4] == '1'))
	{
		set_SP_pulseWidth_unit_API(uart_buf[i+3]-'A',uart_buf[i+4]);
		return i+5;
	}
	return i;
}

unsigned int SPX0XXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	unsigned int SP = 0;

	SP = (uart_buf[i+4]- '0')*100+(uart_buf[i+5]- '0')*10+(uart_buf[i+6]- '0');
	if(SP <= Pulse_width_MAX)
	{
		set_SP_pulseWidth_API(uart_buf[i+2]-'A',SP);
		return i+7;
	}
	return i;
}

unsigned int TR_CMD(unsigned char *uart_buf,unsigned int i)
{

	read_trigger_mode_API();
	return i+2;
}

unsigned int TRX_CMD(unsigned char *uart_buf,unsigned int i)
{
	if((uart_buf[i+2]-'0') <= trigger_mode_MAX)
	{
		set_trigger_mode_API(uart_buf[i+2]-'0');
		return i+3;
	}
	return i;
}

unsigned int TX_CMD(unsigned char *uart_buf,unsigned int i)
{

	set_horl_mode_API(uart_buf[i+1]);
	return i+2;
}
unsigned int T_CMD(unsigned char *uart_buf,unsigned int i)
{

	read_horl_mode_API();
	return i+1;
}
unsigned int TPX_CMD(unsigned char *uart_buf,unsigned int i)
{

	set_triggerpower_API(uart_buf[i+1]);
	return i+3;
}
unsigned int TP_CMD(unsigned char *uart_buf,unsigned int i)
{

	read_triggerpower_API();
	return i+2;
}

unsigned int DLX0XXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	unsigned int light_delay = 0;

	if((uart_buf[i+2] >= 'A')&&(uart_buf[i+2] <= MAX_CHANNEL_LETTER)&&(uart_buf[i+3] == '0'))
	{
		light_delay = (uart_buf[i+4]- '0')*100+(uart_buf[i+5]- '0')*10+(uart_buf[i+6]- '0');
		if(light_delay <= Light_delay_MAX)
		{
			set_light_delay_API(uart_buf[i+2]-'A',light_delay);
			return i+7;
		}
	}
	return i;
}

unsigned int DCX0XXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	unsigned int camera_delay = 0;

	if((uart_buf[i+2] >= 'A')&&(uart_buf[i+2] <= MAX_CHANNEL_LETTER)&&(uart_buf[i+3] == '0'))
	{
		camera_delay = (uart_buf[i+4]- '0')*100+(uart_buf[i+5]- '0')*10+(uart_buf[i+6]- '0');
		if(camera_delay <= Camera_delay_MAX)
		{
			set_camera_delay_API(uart_buf[i+2]-'A',camera_delay);
			return i+7;
		}
	}
	return i;
}

unsigned int DC0XXX_CMD(unsigned char *uart_buf,unsigned int i)
{
	unsigned int camera_delay = 0;

	if(uart_buf[i+2] == '0')
	{
		camera_delay = (uart_buf[i+3]- '0')*100+(uart_buf[i+4]- '0')*10+(uart_buf[i+5]- '0');
		if(camera_delay <= 999)
		{
			set_AllCamera_delay_API(camera_delay);
			return i+6;
		}
	}
	return i;
}

unsigned int DLX_CMD(unsigned char *uart_buf,unsigned int i)
{
	if((uart_buf[i+2] >= 'A')&&(uart_buf[i+2] <= MAX_CHANNEL_LETTER))
	{
		read_light_delay_API(uart_buf[i+2]-'A');
		return i+3;
	}
	return i;
}

unsigned int DCX_CMD(unsigned char *uart_buf,unsigned int i)
{
	if((uart_buf[i+2] >= 'A')&&(uart_buf[i+2] <= MAX_CHANNEL_LETTER))
	{
		read_camera_delay_API(uart_buf[i+2]-'A');
		return i+3;
	}
	return i;
}

unsigned int DC_CMD(unsigned char *uart_buf,unsigned int i)
{
	read_AllCamera_delay_API();
	return i+2;
}

unsigned int CTX_CMD(unsigned char *uart_buf,unsigned int i)
{
	if((uart_buf[i+2] == '0')||(uart_buf[i+2] == '1'))
	{
		set_trigger_UpRoDpwm_API(uart_buf[i+2]-'0');
		return i+3;
	}
	return i;
}

unsigned int CT_CMD(unsigned char *uart_buf,unsigned int i)
{
	read_trigger_UpRoDpwm_API();
	return i+2;
}

unsigned int CST_CMD(unsigned char *uart_buf,unsigned int i)
{
	retun_CST_API();
	return i+2;
}


unsigned int S_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	
	if(uart_buf[i+2] == '#')
	{
		if(uart_buf[i+1] == 'T')
		{
			#if (ST0XXX_ENABLE == 1)
				return ST_CMD(uart_buf,i);
			#endif
		}
		if(uart_buf[i+1] == 'W')
		{
			#if (SWXXXX_ENABLE == 1)
				return SW_CMD(uart_buf,i);
			#endif
		}
		if((uart_buf[i+1] >= 'A')&&(uart_buf[i+1] <= MAX_CHANNEL_LETTER))
		{
			#if (SX0XXX_ENABLE == 1)
				return SX_CMD(uart_buf,i);
			#endif
		}		
	}
	
	if(uart_buf[i+3] == '#')
	{
		if((uart_buf[i+1] == 'P')&&(uart_buf[i+2] >= 'A')&&(uart_buf[i+2] <= MAX_CHANNEL_LETTER))
		{
			#if (SPX0XXX_ENABLE == 1)
			return SPX_CMD(uart_buf,i);
			#endif
		}		
	}
	
	if(uart_buf[i+4] == '#')
	{
		if(uart_buf[i+1] == 'A')
		{
			#if (SAVE_ENABLE == 1)
				return SAVE_CMD(uart_buf,i);
			#endif
		}
		if((uart_buf[i+1] == 'P')&&(uart_buf[i+2] == 'U')&&(uart_buf[i+3] >= 'A')&&(uart_buf[i+3] <= MAX_CHANNEL_LETTER))
		{
			#if (SPUXX_ENABLE == 1)
			return SPUX_CMD(uart_buf,i);
			#endif
		}
		if((uart_buf[i+1] >= '1')&&(uart_buf[i+1] <= '8'))
		{
			return SXXX_CMD(uart_buf,i);
		}		
	}

	if(uart_buf[i+5] == '#')
	{
		if((uart_buf[i+1] == 'P')&&(uart_buf[i+2] == 'U')&&(uart_buf[i+3] >= 'A')&&(uart_buf[i+3] <= MAX_CHANNEL_LETTER))
		{
			#if (SPUXX_ENABLE == 1)
			return SPUXX_CMD(uart_buf,i);
			#endif
		}		
	}
	
	if(uart_buf[i+6] == '#')
	{
		if(uart_buf[i+1] == 'T')
		{
			#if (ST0XXX_ENABLE == 1)
				return ST0XXX_CMD(uart_buf,i);
			#endif
		}
		if(uart_buf[i+1] == 'W')
		{
			if(uart_buf[i+2] == 'T'){
				#if (SWTRIG_ENABLE == 1)
					return SWTRIG_CMD(uart_buf,i);
				#endif
			}
			if((uart_buf[i+2] >= '0')&&(uart_buf[i+2] <= '9')){
				#if (SWXXXX_ENABLE == 1)
					return SWXXXX_CMD(uart_buf,i);
				#endif
			}
		}
		if((uart_buf[i+1] >= 'A')&&(uart_buf[i+1] <= MAX_CHANNEL_LETTER))
		{
			#if (SX0XXX_ENABLE == 1)
				return SX0XXX_CMD(uart_buf,i);
			#endif
		}		
	}
	
	if(uart_buf[i+7] == '#')
	{
		if((uart_buf[i+1] == 'P')&&(uart_buf[i+3] == '0')&&(uart_buf[i+2] >= 'A')&&(uart_buf[i+2] <= MAX_CHANNEL_LETTER))
		{
			#if (SPX0XXX_ENABLE == 1)
			return SPX0XXX_CMD(uart_buf,i);
			#endif
		}		
	}
	return i;
}

unsigned int T_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	#if (TRX_ENABLE == 1)
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] == '#'))   //TR#
	{
		return TR_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] >= '0')&&(uart_buf[i+2] <= '4')&&(uart_buf[i+3] == '#'))   //TRX#
	{
		return TRX_CMD(uart_buf,i);
	}
	#endif
	#if (TX_ENABLE == 1)
	if(((uart_buf[i+1] == 'L')||(uart_buf[i+1] == 'H'))&&(uart_buf[i+2] == '#'))   //TX#
	{
		return TX_CMD(uart_buf,i);
	}
	if(uart_buf[i+1] == '#')  //T#
	{
		return T_CMD(uart_buf,i);
	}
	#endif
	#if (TPX_ENABLE == 1)
	if((uart_buf[i+1] == 'P')&&(uart_buf[i+2] >= '0')&&(uart_buf[i+2] <= '1')&&(uart_buf[i+3] == '#'))   //TPX#
	{
		return TPX_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'P')&&(uart_buf[i+2] == '#'))   //TP#
	{
		return TP_CMD(uart_buf,i);
	}
	#endif
	return i;
}

unsigned int P_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	#if (PRCL_ENABLE == 1)
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] == 'C')&&(uart_buf[i+3] == 'L')&&(uart_buf[i+4] == '#'))   //PRCL#
	{
		return TPX_CMD(uart_buf,i);
	}
	#endif
	#if (PRWX_ENABLE == 1)
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] == 'W')&&(uart_buf[i+3] >= 'A')&&(uart_buf[i+3] <= MAX_CHANNEL_LETTER)&&(uart_buf[i+4] == '#'))   //PRWX#
	{
		return TPX_CMD(uart_buf,i);
	}
	#endif
	#if (PRNXXX_ENABLE == 1)
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] == 'N')&&(uart_buf[i+3] >= 'A')&&(uart_buf[i+3] <= MAX_CHANNEL_LETTER)&&(uart_buf[i+6] == '#'))   //PRNXXX#
	{
		return TPX_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] == 'N')&&(uart_buf[i+3] >= 'A')&&(uart_buf[i+3] <= MAX_CHANNEL_LETTER)&&(uart_buf[i+4] == '#'))   //PRNX#
	{
		return TPX_CMD(uart_buf,i);
	}
	#endif
	#if (PRENCLRX_ENABLE == 1)
	if((uart_buf[i+1] == 'R')&&(uart_buf[i+2] == 'E')&&(uart_buf[i+3] == 'N')&&(uart_buf[i+4] == 'C')
		&&(uart_buf[i+5] == 'L')&&(uart_buf[i+6] == 'R')&&(uart_buf[i+7] >= 'A')&&(uart_buf[i+7] <= MAX_CHANNEL_LETTER)&&(uart_buf[i+8] == '#'))   //PRCL#
	{
		return TPX_CMD(uart_buf,i);
	}
	#endif
	return i;
}

unsigned int D_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	#if (DLX0XXX_ENABLE == 1)
	if((uart_buf[i+1] == 'L')&&(uart_buf[i+7] == '#'))  //DLX0XXX# 
	{
		return DLX0XXX_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'L')&&(uart_buf[i+3] == '#'))  //DLX# 
	{
		return DLX_CMD(uart_buf,i);
	}
	#endif
	#if (DCX0XXX_ENABLE == 1)
	if((uart_buf[i+1] == 'C')&&(uart_buf[i+7] == '#'))  //DCX0XXX# 
	{
		return DCX0XXX_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'C')&&(uart_buf[i+3] == '#'))  //DCX# 
	{
		return DCX_CMD(uart_buf,i);
	}
	#endif
	#if (DC0XXX_ENABLE == 1)
	if((uart_buf[i+1] == 'C')&&(uart_buf[i+6] == '#'))  //DC0XXX# 
	{
		return DC0XXX_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'C')&&(uart_buf[i+2] == '#'))  //DC# 
	{
		return DC_CMD(uart_buf,i);
	}
	#endif
	return i;
}

unsigned int C_cmdhandle(unsigned char *uart_buf,unsigned int i)
{
	#if (CTX_ENABLE == 1)
	if((uart_buf[i+1] == 'T')&&(uart_buf[i+3] == '#'))  //DLX# 
	{
		return CTX_CMD(uart_buf,i);
	}
	if((uart_buf[i+1] == 'T')&&(uart_buf[i+2] == '#'))  //DC# 
	{
		return CT_CMD(uart_buf,i);
	}
	#endif
	if((uart_buf[i+1] == 'S')&&(uart_buf[i+2] == 'T'))  //CST 
	{
		return CST_CMD(uart_buf,i);
	}
	return i;
}

unsigned int Old_cmd_handle_func(unsigned char *uart_buf,unsigned int i)
{

	switch (uart_buf[i])
	{
		case 'S':
			i = S_cmdhandle(uart_buf,i);
			break;
		case 'T':
			i = T_cmdhandle(uart_buf,i);
			break;
		case 'P':
			i = P_cmdhandle(uart_buf,i);
			break;
		case 'D':
			i = D_cmdhandle(uart_buf,i);
			break;
		case 'C':
			i = C_cmdhandle(uart_buf,i);
			break;
		case '$':
			i = Extend_write_cmdhandle(uart_buf,i);
			break;
		case '@':
			i = Extend_read_cmdhandle(uart_buf,i);
			break;
	}
	return i;
}


