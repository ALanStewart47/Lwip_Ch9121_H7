#ifndef __COMMUNICAT_H
#define __COMMUNICAT_H

#include "stdio.h"
#include "stdint.h"
#include "wizchip_conf.h"
#include "bsp_w5500.h"


//#define UDP_RBUF_SIZE	290
//#define UDP_SBUF_SIZE	290
//#define TCPS_RBUF_SIZE  50
//#define TCPS_SBUF_SIZE	50

//#define UDP_PORT		6144	//UDP端口号
//#define TCP_PORT		6600	//TCP端口号



enum Mode_type{
				SP_mode=0,        //频闪模式
				no_mode=1,        //空
				Can_pro_mode=2,   //可编程模式
				DP_mode=3		  //数字模式
			  };


//extern uint8_t udp_senbuf[UDP_SBUF_SIZE];//udp发送缓存

									
extern uint8_t  broadcastip[4];//广播地址

extern uint8_t  udp_destip[4];//udp电脑端的ip

extern uint16_t udp_destport;//udp电脑端的端口


//TCP全局变量
/*
extern uint8_t tcps_recbuf[TCPS_RBUF_SIZE];//tcp接收缓存
extern uint8_t tcps_senbuf[TCPS_SBUF_SIZE];//tcp发送缓存
*/

//网口参数
/*
extern uint8_t devname[7];//设备名称
extern uint8_t CH_mac[7] ;
extern uint8_t mac[6];//MAC地址
extern uint8_t ip[4]; //IP地址
extern uint8_t sn[4]; //子网掩码
extern uint8_t gw[4]; //网关
extern uint8_t dns[4]; //DNS服务器地址
extern dhcp_mode dhcp ;//动静态分配 
extern uint8_t g_dhcp_state ;//DHCP开关标志
extern uint8_t dhcpmsgbuf[600];
*/



/*

extern uint16_t CH1_pwm;  //LED_PWM
extern uint16_t CH1_tim;
extern uint16_t CH1_DL;  
extern uint8_t HorL;     //常亮或常灭
extern uint8_t DigOrSP;
extern uint8_t change_mode;    //改变模式

extern uint8_t U1Rx_Buff[14];  //串口1接收buff
extern uint8_t U1Tx_Buff[14];  //串口1发送buff
extern uint8_t U1Rx_data;

extern uint8_t U2Rx_Buff[14];  //串口2接收buff
extern uint8_t U2Tx_Buff[14];  //串口2发送buff
extern uint8_t U2Rx_data;

extern uint8_t U3Rx_Buff[8];  //串口3接收buff
extern uint8_t U3Tx_Buff[45];  //串口3发送buff
extern uint8_t U3Rx_data;

extern uint8_t color;
extern uint8_t Now_color;

extern uint8_t temp1;
extern uint8_t temp2;
extern uint8_t temp3;

extern uint16_t PR_buff[33][2];
extern uint8_t PR_Now_Step;
extern uint8_t PRXH;
extern uint8_t PR_Save_flag,PR_Erase_cnt; 

extern uint8_t set_fil_flag;
extern uint8_t update_fil_flag;
extern uint8_t update_DL_flag;

extern uint8_t buff_temp;
extern uint8_t U3len;
extern uint8_t RX3_Flag;



extern void U1RX_parse(void);
//extern void UDP_app(void);
extern void TCPS_app(void); 
extern void U3RX_parse(void);
extern void Temp_update(void);
extern void PWM_update(void);
extern void fIL_update(void);


*/



#endif

