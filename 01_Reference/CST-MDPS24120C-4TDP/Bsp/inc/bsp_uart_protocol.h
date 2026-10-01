/*
*********************************************************************************************************
*
*	模块名称 : 串口通讯协议处理模块
*	文件名称 : bsp_uart_protocol.h
*	说    明 : 头文件
*
*********************************************************************************************************
*/

#ifndef _BSP_UART_PROTOCOL_H_
#define _BSP_UART_PROTOCOL_H_

#include "main.h"


// uart time out 
#define UART_TIME_OUT			5

//cmd head 
#define OPT_FF 	    			0xFF
#define OPT_FE 	    			0xFE
#define OPT_FA					0xFA
#define BOOTLOADER_HEAD 		0x72


#define OPT_FD_CMD              0xFD

#define OPT_FF_CMD				OPT_FF 	    			
#define OPT_FF_CMD_LENGTH       6

#define OPT_FE_CMD				OPT_FE 	 

#define PROCO_LIGHT_CHANNEL_NUM		8

/* UDP  TCP  端口号和缓存大小*/
//#define UDP_RBUF_SIZE	290
//#define UDP_SBUF_SIZE	290
////#define TCPS_RBUF_SIZE  50
//#define TCPS_SBUF_SIZE	50

#define UDP_RBUF_SIZE	290
#define UDP_SBUF_SIZE	290
#define TCPS_RBUF_SIZE  900
#define TCPS_SBUF_SIZE	50

#define UDP_PORT		6144	//UDP端口号
#define TCP_PORT		8000	//TCP端口号



//modify the number can connet more in dhcp
#define DHCP_CONNET_NUMBER      3//3

/* 获取控制器IP sn gw 等信息的指令长度 */
#define OPT_GET_NET_CONFIG_DATA_LENGTH      11
#define OPT_SET_NET_CONFIG_DATA_LENGTH      21

#define OPT_TYPE_FIRST          0x60
#define OPT_TYPE_SECOND         0x24

#define OPT_VERSION_FIRST       0x01
#define OPT_VERSION_SECOND      0x01

#define OPT_CHANNEL_NUM         0x08
    
#define OPT_UDP_BACK            0x01
#define OPT_UDP_RADIO_CHECK     0x00
    


#define OPT_FF_READ_PROPERTY    0x10 
#define OPT_FD_READ_PROPERTY    0x01   

/**************** TCP/IP ****************************/

#define OPT_TCP_FF_CMD_LENGTH          6

/* 特征字 */
#define OPT_TCP_FF_CMD              0xFF
#define OPT_TCP_FE_CMD              0xFE

/* 命令字 */
#define OPT_TCP_FF_CMD_SET_LIGHT_VALUE      		0x01
#define OPT_TCP_FF_CMD_CHANNEL_SWITCH       		0x05

#define OPT_TCP_FF_CMD_SOFT_TRIG            		0x06

#define OPT_READ_CTROLLER_CONFIG      				0x10

#define OPT_TCP_FF_CMD_READ_LIGHT_VALUE     		0x11
#define OPT_TCP_FF_CMD_READ_SOFT_TRIG       		
#define OPT_TCP_FF_CMD_READ_SETTING_PARA    		0x85

#define OPT_TCP_FF_CMD_SET_MODE						0x22
#define OPT_TCP_FF_CMD_READ_MODE					0x16

#define OPT_TCP_FF_CMD_SET_POWER_ON_STATUS			0x29
#define OPT_TCP_FF_CMD_READ_POWER_ON_STATUS			0x03	//ff 85 0x 03

#define OPT_TCP_FF_CMD_READ_ALL_CHANNEL_SETTINGS	0x85

#define OPT_TCP_FF_CMD_SET_TRIG_ON_TIME				0x02
#define OPT_TCP_FF_CMD_READ_TRIG_ON_TIME			0x12

#define OPT_TCP_FF_CMD_SET_TRIG_ON_UINT				0x4B
#define OPT_TCP_FF_CMD_READ_TRIG_ON_UINT			0x11  //ff 85 0x 11 

#define OPT_TCP_FF_CMD_SET_TRIG_DELAY_TIME			0x07
#define OPT_TCP_FF_CMD_READ_TRIG_DELAY_TIME			0x1A  //1US 

#define OPT_TCP_FF_CMD_SET_TRIG_DELAY_UINT			0x4E
#define OPT_TCP_FF_CMD_READ_TRIG_DELAY_UINT			0x2D  //ff 85 0x 2D 


#define OPT_TCP_FF_CMD_READ_NET_CONFIG				0x18


#define OPT_TCP_FE_CMD_MULTI_SET_LIGHT_VALUE      	0x01
#define OPT_TCP_FE_CMD_MULTI_CHANNEL_SWITCH       	0x05
#define OPT_TCP_FE_CMD_MULTI_SOFT_TRIG            	0x06

#define OPT_FA_CMD_PROG_TRIG_SEQ_TABLE				0x01	//
#define OPT_FA_CMD_PROG_TRIG_MODE_SWITCH			0x02	//
#define OPT_FA_CMD_PROG_TRIG_SET_CUR_SEQ			0x03	//
#define OPT_FA_CMD_PROG_TRIG_RESET_SEQ				0x04	//
#define OPT_FA_CMD_PROG_TRIG_READ_SEQ_TABLE			0x11	//
#define OPT_FA_CMD_PROG_TRIG_READ_CUR_SEQ			0x12	//
#define OPT_FA_CMD_PROG_TRIG_READ_SEQ_NUM			0x13	//
#define OPT_FA_CMD_PROG_TRIG_READ_MODE				0x14	//

#define OPT_TCP_FE_CMD_END                        	0xFF

#define OPT_SOFT_TRIG_TIME_MAX              		3000



/**
 * 错误标志
 * 用于g_ErrFlag赋值和显示函数
*/
typedef enum
{
	R_OCP_FLAG	=	1,
	G_OCP_FLAG	,
	B_OCP_FLAG	,
	POWERDOWN_FLAG	 
}ERROR_STATE_FLAG;



typedef enum
{
	PULSE_WIDTH_NUM_1	=	0,
	PULSE_WIDTH_NUM_2			
}PLUSE_WIDTH_CHANNEL_NUM;


typedef enum
{
	CMD_ALL_CH = 0	,
	CMD_CH1			,
	CMD_CH2			,
	CMD_CH3			,
	CMD_CH4			,
	CMD_CH5			,
	CMD_CH6			,
	CMD_CH7			,
	CMD_CH8			,
}CMD_CHANNEL_NUMBER_TYPE;

typedef enum 
{
	ANALYZE_ERROR 		=	0,
	ANALYZE_OK
}PROTOCOL_STATE_FLAG;

/*	不同光源的参数	*/
typedef struct
{
	GPIO_TypeDef* power_gpio;		/*	电源控制引脚	*/
	uint16_t power_pin;
	GPIO_TypeDef* channel_gpio;	/*	光源控制引脚	*/
	uint16_t channel_pin;	
	/**/
	uint8_t 	channel;					/*	光源通道序号	-用于显示和通讯之用	*/
	uint8_t		State;						/*	光源开关状态	*/
	uint16_t 	value;						/*	光源亮度	*/
	uint8_t		TrigState;					/*	触发状态	*/
}LIGHT_DATA;

// 1. 定义发送回调类型
typedef void (*protocol_send_func_t)(uint8_t sn, uint8_t *buf, uint16_t len);

/*
void analyze_FF_Protocol(uint8_t * _ucBuf ,	uint16_t _uslen);

void analyze_FE_Protocol(uint8_t * _ucBuf , uint16_t _us_len);

//void uart_protocol_handle(COM_PORT_E com_num);

void net_protocol_handle(void);

void bsp_RunBootLoader(uint8_t * _ucBuf , uint16_t _uslen );
*/

void protocol_parse_entry(uint8_t *buf, uint16_t len, protocol_send_func_t send_func, uint8_t sn);

#endif

/***************************** (END OF FILE) *********************************/

