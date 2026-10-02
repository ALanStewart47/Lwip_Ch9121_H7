#ifndef __PROTOCOL_PUBLIC_H
#define __PROTOCOL_PUBLIC_H

#ifdef __cplusplus
extern "C" {
#endif
#include "protocol_share_header.h"

#define		CHANNEL_NUM		8              //Number of channels
#define    MAX_CHANNEL_LETTER		'A'-1+CHANNEL_NUM
#define    SUM_SIZE			300

#define    RECIPE_SUM					1
#define    TriggerSource_SUM			1
#define    Line_SUM						64


typedef struct UART_Prepare_Buf{
	unsigned char state;
	unsigned int available_length;
	unsigned char rx_buffer[SUM_SIZE];
	unsigned int data_start_position;
	unsigned int data_end_position;
	unsigned char data_type; 	//TCP UDP
}UART_Prepare_Buf;

//********Digital controller data**************//
typedef struct Digital_Mode{
	unsigned int brightness[CHANNEL_NUM];
	unsigned char Channel_Switch[CHANNEL_NUM];
	unsigned char Digital_trigger_mode;			// 1 Light on 0 light off
	unsigned int Color_Temperature[CHANNEL_NUM];
	unsigned char brightness_level;			// 1 999 0 255
}Digital_Mode;

//********Strobe controller data**************//
typedef struct Strobe_Mode{
	unsigned int Pulse_width[CHANNEL_NUM];
	unsigned int Light_delay[CHANNEL_NUM];
	unsigned int Camera_delay[CHANNEL_NUM];
	unsigned int Trigger_cycle;
	unsigned char trigger_mode;
	unsigned char Camera_trigger_mode;		//0 Rising edge 1 Descending edge
	unsigned char Pulse_width_unit;			//Pulse width unit in strobe mode
	unsigned int Trigger_filtering;		//Filter value of input signal
}Strobe_Mode;

//********Shared data area**************//
typedef struct Public_Function{
	unsigned int mode; 				//0 Normal mode 1¡¢2 Programmable mode
	unsigned int Baud_rate;					//0 4800 1 9600 2 14400 3 19200 4 28800 5 38400 6 57600 7 115200
	unsigned int Input_triggers_number[CHANNEL_NUM];		//The number of times the trigger signal
	unsigned int LightOutput_triggers_number[CHANNEL_NUM];	//Light output times
	unsigned int CameraOutput_triggers_number[CHANNEL_NUM];	//Camera output times
	unsigned int SoftTrig_Width[CHANNEL_NUM];	//softwareTrig Pulse width
	unsigned int TempeThreshold[CHANNEL_NUM];	//softwareTrig Pulse width
	unsigned int SlavesNumb; 				//
}Public_Function;

//********Programmable data area**************//
typedef struct Programmable_Function{
	unsigned int Prog_data[RECIPE_SUM][TriggerSource_SUM][Line_SUM][CHANNEL_NUM];
	unsigned int Prog_data_PulseWidth[RECIPE_SUM][TriggerSource_SUM][Line_SUM];		
	unsigned int Prog_Camera_output[RECIPE_SUM][TriggerSource_SUM][Line_SUM];
	unsigned int Prog_Total_steps[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Current_steps[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Trigger_mode[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_trigger_interval[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Camera_delay[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Light_delay[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Step_switch[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Start_Steps[RECIPE_SUM][TriggerSource_SUM];
	unsigned int Prog_Stop_Steps[RECIPE_SUM][TriggerSource_SUM];
}Programmable_Function;

typedef struct Controller_Data{
	Digital_Mode Digital_data;
	Strobe_Mode Strobe_data;
	Public_Function Public_Data;
	Programmable_Function Programmable_Data;
}Controller_Data;

extern Controller_Data Controller_Data_a;


void init_protocol_para(void);
void input_data(unsigned char *data,unsigned int length);
unsigned char* get_prepare_tx_buffer(unsigned int *length);
void analysis_command(void);





void clear_tx_buff( unsigned int num);
void set_state_clean(unsigned char channel,unsigned int value);
extern unsigned char cmd_up[15];
extern unsigned char prepare_tx_buffer[SUM_SIZE] ;
extern unsigned int prepare_tx_length ;
extern UART_Prepare_Buf prepare_data2; 
void set_state_clean_(unsigned char channel,unsigned int value);
void f3_data_up(void);
#ifdef __cplusplus
}
#endif

#endif 
