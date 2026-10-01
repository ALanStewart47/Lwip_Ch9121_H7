#include "main.h"


#include "communicat.h"


#include "bsp_w5500.h"
#include "stdio.h"
#include "socket.h"
#include "basic_crc.h"
#include "dhcp.h"
#include "ModuleConfig.h"
/*

typedef struct 
{
	char cmdbuf[17];	//用来保存指令内容
	char cmd_len;		//指令长度
	char data_start_posi;//变数起始位置
	char data_end_posi;	//变终点位置
	char data_cmp_posi;	//当前与指令比较的位置 
}CMDTypdef;

#define SENDBUF_SIZE  255
#define CMDLEN  100
//uint8_t version[5]="0110";
uint16_t CH1_Val_Tmp = 0;
uint16_t Filter_value=3;


CMDTypdef CMD_SA0xxx = {"SA0\xFF\xFF\xFF#"};
CMDTypdef CMD_SPAxxx = {"SPA\xFF\xFF\xFF\xFF#"};
CMDTypdef CMD_DLA0xxx = {"DLA0\xFF\xFF\xFF#"};


CMDTypdef CMD_SA = {"SA#"};
CMDTypdef CMD_SPA = {"SPA#"};
CMDTypdef CMD_DLA = {"DLA#"};

CMDTypdef CMD_TH = {"TH#"};
CMDTypdef CMD_TL = {"TL#"};
CMDTypdef CMD_T = {"T#"};
CMDTypdef CMD_CST = {"CST"};
CMDTypdef CMD_VER = {"VER#"};
CMDTypdef CMD_SAH = {"SAH#"};
CMDTypdef CMD_SAL = {"SAL#"};

CMDTypdef CMD_SABCD = {"SABCD#"};

CMDTypdef CMD_TRx = {"TR\xFF#"};
CMDTypdef CMD_TR = {"TR#"};

*/
/*
CMDTypdef CMD_Upgrade = {0x72,0x68,0xaa,0xff,0xff,0x16};
CMDTypdef CMD_VER_inf = {"\x72\x68\xbb\x16"};

uint16_t PR_buff[33][2];   //可编程数据
uint8_t PR_Now_Step =0;    //当前步数
uint8_t PRXH = 0;   //触发序号
uint8_t PR_Save_flag = 0,PR_Erase_cnt=0;   //可编程数据保存

uint8_t U1Rx_Buff[14];  //串口1接收buff
uint8_t U1Tx_Buff[14];  //串口1发送buff
uint8_t U1Rx_data;
uint8_t U1len=0;
uint8_t RX_Flag=0;
uint16_t CH1_pwm=0;  //LED_PWM
uint16_t CH1_tim=0;  //LED_脉宽
uint16_t CH1_DL=0;  //LED_光源延时
uint8_t HorL=0;     //常亮或常灭
uint8_t DigOrSP=0;  //数字还是屏闪   3为数字 0为频闪 2为可编程
uint8_t usart1_data = 0;


uint8_t U2Rx_Buff[14];  //串口2接收buff
uint8_t U2Tx_Buff[14];  //串口2发送buff
uint8_t U2Rx_data;
uint8_t U2len=0;
uint8_t RX2_Flag=0;
uint8_t usart2_data = 0;


uint8_t U3Rx_Buff[8];  //串口3接收buff
uint8_t U3Tx_Buff[45];  //串口3发送buff
uint8_t U3Tx_Buff2[45]; 
uint8_t U3Rx_data;
uint8_t U3len=0;
uint8_t RX3_Flag=0;

uint8_t change_mode=0;    //改变模式
uint8_t color=0;		  //设置灯颜色
uint8_t Now_color=0;      //当前灯颜色

uint8_t temp1=0;   //温度1
uint8_t temp2=0;   //温度2
uint8_t temp3=0;   //温度3
uint8_t temp4=0;
uint8_t buff_temp=0;

uint8_t set_fil_flag=0;
uint8_t update_fil_flag=0;
uint8_t update_DL_flag=0;

*/


//UDP全局变量
//uint8_t udp_recbuf[UDP_RBUF_SIZE];//udp接收缓存


//uint8_t udp_senbuf[UDP_SBUF_SIZE]={ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x21,0x21,0x01,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x01,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x02,0x01,0xB8,0x0B,\
//									0xC0,0xA8,0x01,0x64,0xD0,0x07,0x80,0x25,0x00,0x00,0x08,0x01,0x04,0x01,0x00,0x04,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,\
//									0x01,0x00,0x00,0xC8,0x19,0xC0,0xA8,0x01,0x64,0xE8,0x03,0x00,0x4B,0x00,0x00,0x08,0x01,0x04,0x01,0x00,\
//									0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
//									0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
//									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//udp发送缓存

	/*							
uint8_t  broadcastip[4]={255,255,255,255};//广播地址

uint8_t  udp_destip[4];//udp电脑端的ip

uint16_t udp_destport;//udp电脑端的端口
*/
//uint8_t  udp_rce_flag = 0;	//udp接收标志 0：没收到UDP数据  1：收到UDP数据
//uint16_t  udp_rec_len = 0;	//接收到的数据长度

//TCP全局变量
//uint8_t tcps_recbuf[TCPS_RBUF_SIZE];//tcp接收缓存
//uint8_t tcps_senbuf[TCPS_SBUF_SIZE];//tcp发送缓存

/*
//网口参数
uint8_t devname[7] = {'P','O','E','0','0','1','\0'};//设备名称
uint8_t CH_mac[7] = {0x02,0x03,0x04,0x05,0x06,0x07};
uint8_t mac[6]={0};//MAC地址
uint8_t ip[4]={192,168,1,188}; //IP地址
uint8_t sn[4]={255,255,255,0}; //子网掩码

//uint8_t gw[4]={192,168,1,1}; //网关
uint8_t gw[4]={0,0,0,0}; //网关

uint8_t dns[4]={0,0,0,0}; //DNS服务器地址
dhcp_mode dhcp = NETINFO_DHCP;//动静态分配 
uint8_t g_dhcp_state = 1;//DHCP开关标志
uint8_t dhcpmsgbuf[600];
*/


/*
void UDP_app(void)
{		
	switch(getSn_SR(0))															
	{
		case SOCK_UDP:												   			
		
		if((getSn_RX_RSR(0))>0 && udp_rce_flag == 0)
		{ 
			if( (udp_rec_len = recvfrom(0,udp_recbuf, UDP_RBUF_SIZE, udp_destip,&udp_destport)) > 0)
			{			
				udp_rce_flag = 1;
			}		
			else
			{
//				printf("recvfrom return error\r\n");
			}
		}
		break;
		case SOCK_CLOSED:														
			if (socket(0,Sn_MR_UDP,UDP_PORT,0) < 0)									
			{
//				printf("socket 0 UDP opened fail\r\n");
			}
			else
			{
//				printf("socket 0 Opened, UDP , port %d\r\n",UDP_PORT);			
			}
			break;
			
		default:
			break;
	} 
		
	//接收了UDP数据   //ch9121搜索协议
	if(udp_rce_flag == 1)
	{
		//ETHGET
		if(udp_rec_len == 285)
		{
			if(udp_recbuf[0] == 'C' && udp_recbuf[1] == 'H' && udp_recbuf[2] == '9' && udp_recbuf[3] == '1' && udp_recbuf[4] == '2' && udp_recbuf[5] == '1'  \
			&& udp_recbuf[6] == '_' && udp_recbuf[7] == 'C' && udp_recbuf[8] == 'F'&& udp_recbuf[9] == 'G' && udp_recbuf[10] == '_' && udp_recbuf[11] == 'F'   \
			&& udp_recbuf[12] == 'L' && udp_recbuf[13] == 'A' && udp_recbuf[14] == 'G')
			{				
				if(udp_recbuf[16] == NET_MODULE_CMD_SEARCH)
				{
					memcpy(udp_senbuf, CH9121_CFG_FLAG_1, 15);
					memcpy(udp_senbuf+15, CH9121_NC_84, 2);
					memcpy(udp_senbuf+17, mac, 6);
//					memcpy(udp_senbuf+29, &CH9121_len_12, 4);
					memcpy(udp_senbuf+30, ip, 4);
					memcpy(udp_senbuf+34, devname, 6);
					memcpy(udp_senbuf+40, CH9121_NC_end, 3);
					sendto(0,udp_senbuf,285, broadcastip, udp_destport);
					
				}
				
				if(udp_recbuf[16] == NET_MODULE_CMD_GET)
				{
					memcpy(udp_senbuf, CH9121_CFG_FLAG_1, 15);
					memcpy(udp_senbuf+15, CH9121_NC_82, 2);
					memcpy(udp_senbuf+17, mac, 6);
//					memcpy(udp_senbuf+29, &CH9121_len_204, 1);
					memcpy(udp_senbuf+30, &CH9121_2121, 5);
					memcpy(udp_senbuf+35, devname, 6);
					memcpy(udp_senbuf+62, ip, 4);
					memcpy(udp_senbuf+66, gw, 4);
					memcpy(udp_senbuf+70, sn, 4);
					memcpy(udp_senbuf+74, &g_dhcp_state, 1);
					
					sendto(0,udp_senbuf,285, broadcastip, udp_destport);
					
				}
				
				
				if(udp_recbuf[16] == NET_MODULE_CMD_SET)
				{
					if(udp_recbuf[17] == mac[0] && udp_recbuf[18] == mac[1] && udp_recbuf[19] == mac[2] && udp_recbuf[20] == mac[3] && udp_recbuf[21] == mac[4] && udp_recbuf[22] == mac[5])
					{	
						memcpy(devname, udp_recbuf+35, 6);
						memcpy(ip, udp_recbuf+62, 4);
						memcpy(sn, udp_recbuf+70, 4);
						memcpy(gw, udp_recbuf+66, 4);
						if(udp_recbuf[74] == 0)
						{
							g_dhcp_state = 0;
							dhcp = NETINFO_STATIC;
						}
						else
						{
							g_dhcp_state = 1;
							dhcp = NETINFO_DHCP;
						}
						
						
						
						memcpy(udp_senbuf, CH9121_CFG_FLAG_1, 15);
						memcpy(udp_senbuf+15, CH9121_NC_81, 2);
						memcpy(udp_senbuf+17, mac, 6);
//						memcpy(udp_senbuf+23, &(udp_recbuf[23]), 5);
//						memcpy(udp_senbuf+30, &CH9121_2121, 5);
//						memcpy(udp_senbuf+35, devname, 6);
//						memcpy(udp_senbuf+56, CH_mac, 6);
//						memcpy(udp_senbuf+62, ip, 4);
//						memcpy(udp_senbuf+66, gw, 4);
//						memcpy(udp_senbuf+70, sn, 4);
//						memcpy(udp_senbuf+74, &g_dhcp_state, 1);
						
						memcpy(udp_senbuf+30, &(udp_recbuf[30]), 255);
						
						sendto(0,udp_senbuf,285, broadcastip, udp_destport);
						
						
						//保存参数
						ManSave();
						//重新配置
						W5500_Config();
					}
					
					
					
				}
				
			}
		}

		if(udp_rec_len == 6)
		{
			
			if(udp_recbuf[0] == 'E' && udp_recbuf[1] == 'T' && udp_recbuf[2] == 'H' && udp_recbuf[3] == 'G' && udp_recbuf[4] == 'E' && udp_recbuf[5] == 'T' )
			{								
				
				memcpy(udp_senbuf, "ETHGET", 6);
				memcpy(udp_senbuf+6, devname, 6);
				memcpy(udp_senbuf+12, mac, 6);
				memcpy(udp_senbuf+18, ip, 4);
				memcpy(udp_senbuf+22, sn, 4);
				memcpy(udp_senbuf+26, gw, 4);
				if(dhcp == NETINFO_STATIC)
				{
					g_dhcp_state = 0;
				}
				else
				{
					g_dhcp_state = 1;
				}
				memcpy(udp_senbuf+30,&g_dhcp_state, 1);				
				sendto(0,udp_senbuf,31, broadcastip, udp_destport);
			}
		}


//		}
		  
		//ETHSET
		#if 0
		if(udp_rec_len == 31)
		{
			if(udp_recbuf[0] == 'E' && udp_recbuf[1] == 'T' && udp_recbuf[2] == 'H' && udp_recbuf[3] == 'S' && udp_recbuf[4] == 'E' && udp_recbuf[5] == 'T' \
		 && udp_recbuf[12] == mac[0] && udp_recbuf[13] == mac[1] && udp_recbuf[14] == mac[2] && udp_recbuf[15] == mac[3] && udp_recbuf[16] == mac[4] && udp_recbuf[17] == mac[5])
			{
				memcpy(devname, udp_recbuf+6, 6);
				//memcpy(mac, udp_recbuf+12, 6);
				memcpy(ip, udp_recbuf+18, 4);
				memcpy(sn, udp_recbuf+22, 4);
				memcpy(gw, udp_recbuf+26, 4);
				if(udp_recbuf[30] == 0)
				{
					g_dhcp_state = 0;
					dhcp = NETINFO_STATIC;
				}
				else
				{
					g_dhcp_state = 1;
					dhcp = NETINFO_DHCP;
				}
				
				memcpy(udp_senbuf, "ETHGET", 6);
				memcpy(udp_senbuf+6,udp_recbuf+6,25);				
				sendto(0,udp_senbuf,31, broadcastip, udp_destport);
				
				//保存参数
				ManSave();
				//重新配置
				W5500_Config();
			}		
		}
		
		//SA0XXX#
		if(udp_rec_len == 7)
		{
			if(udp_recbuf[0] == 'S' && udp_recbuf[1] == 'A' && udp_recbuf[6] == '#' )
			{
				REG_CH1VAL = ( (udp_recbuf[3]-48)*100+(udp_recbuf[4]-48)*10+(udp_recbuf[5]-48));
				if(REG_CH1VAL > 255)
				{
					REG_CH1VAL = 255;
				}
				memcpy(udp_senbuf, "a", 1);
				sendto(0,udp_senbuf,1, broadcastip, udp_destport);
			}
		}
		
		//SA#
		if(udp_rec_len == 3)
		{
			
			if(udp_recbuf[0] == 'S' && udp_recbuf[1] == 'A' && udp_recbuf[2] == '#' )
			{				
				memcpy(udp_senbuf, "a0", 2);
				udp_senbuf[2] = REG_CH1VAL/100+48;
				udp_senbuf[3] = REG_CH1VAL/10%10+48;
				udp_senbuf[4] = REG_CH1VAL%10+48;				
				sendto(0,udp_senbuf,5, broadcastip, udp_destport);
			}
			
			if(udp_recbuf[0] == 'T' && udp_recbuf[1] == 'H' && udp_recbuf[2] == '#' )
			{			
				REG_CH1_SW = 0x0001;
				memcpy(udp_senbuf, "h", 1);
				sendto(0,udp_senbuf,1, broadcastip, udp_destport);
			}
			
			if(udp_recbuf[0] == 'T' && udp_recbuf[1] == 'L' && udp_recbuf[2] == '#' )
			{			
				REG_CH1_SW = 0x0000;
				memcpy(udp_senbuf, "l", 1);
				sendto(0,udp_senbuf,1, broadcastip, udp_destport);
			}
		}
		//T#
		if(udp_rec_len == 2)
		{
			if(udp_recbuf[0] == 'T' && udp_recbuf[1] == '#')
			{
				if(REG_CH1_SW & 0x0001)
				{
					memcpy(udp_senbuf, "H", 1);
					sendto(0,udp_senbuf,1, broadcastip, udp_destport);
				}
				else
				{
					memcpy(udp_senbuf, "L", 1);
					sendto(0,udp_senbuf,1, broadcastip, udp_destport);
				}
			}
		}
		#endif		
		modbus_analysis(udp_recbuf,udp_rec_len,0);
		
		memset(udp_recbuf,0,UDP_RBUF_SIZE);
		memset(udp_senbuf,0,UDP_SBUF_SIZE);
		udp_rce_flag = 0;
	}
}
*/
/*
void TCPS_app(void)										// Socket
{
	int tcp_rec_len = 0;
	int len = 0;
	switch(getSn_SR(1))									// ??socket0???
	{
		case SOCK_INIT:									// Socket???????(??)??
			
			listen(1);									// ???????????,???????
		break;
		case SOCK_ESTABLISHED:							// Socket????????
						
			len = getSn_RX_RSR(1);						// ??W5500???????????,Sn_RX_RSR??????????????????
		
			if(len>0)
			{
				//memset(tcps_recbuf,0,TCPS_RBUF_SIZE);
				
				tcp_rec_len = recv(1,tcps_recbuf,TCPS_RBUF_SIZE);						// W5500??????????,???SPI???MCU

				modbus_analysis(tcps_recbuf,tcp_rec_len,1);		//
									
				if(tcp_rec_len == 6)
				{
					if(tcps_recbuf[0] == 'E' && tcps_recbuf[1] == 'T' && tcps_recbuf[2] == 'H' && tcps_recbuf[3] == 'G' && tcps_recbuf[4] == 'E' && tcps_recbuf[5] == 'T' )
					{				
						memcpy(tcps_senbuf, "ETHGET", 6);
						memcpy(tcps_senbuf+6, devname, 6);	
						memcpy(tcps_senbuf+12, mac,6);						
						memcpy(tcps_senbuf+18, ip, 4);
						memcpy(tcps_senbuf+22, sn, 4);
						memcpy(tcps_senbuf+26, gw, 4);
						if(dhcp == NETINFO_STATIC)
						{
							g_dhcp_state = 0;
						}
						else
						{
							g_dhcp_state = 1;
						}
						memcpy(tcps_senbuf+30,&g_dhcp_state, 1);						
						send(1,tcps_senbuf,31);						
					}
				}
				 
				
				if(tcp_rec_len == 31)
				{
					if(tcps_recbuf[0] == 'E' && tcps_recbuf[1] == 'T' && tcps_recbuf[2] == 'H' && tcps_recbuf[3] == 'S' && tcps_recbuf[4] == 'E' && tcps_recbuf[5] == 'T' \
		&& tcps_recbuf[12] == mac[0] && tcps_recbuf[13] == mac[1] && tcps_recbuf[14] == mac[2] && tcps_recbuf[15] == mac[3] && tcps_recbuf[16] == mac[4] && tcps_recbuf[17] == mac[5])
					{
						memcpy(devname,tcps_recbuf+6,6);
						//memcpy(mac, tcps_recbuf+12, 6);
						memcpy(ip, tcps_recbuf+18, 4);
						memcpy(sn, tcps_recbuf+22, 4);
						memcpy(gw, tcps_recbuf+26, 4);
						if(tcps_recbuf[30] == 0)
						{
							g_dhcp_state = 0;
							dhcp = NETINFO_STATIC;
						}
						else
						{
							g_dhcp_state = 1;
							dhcp = NETINFO_DHCP;
						}
						
						memcpy(tcps_senbuf, "ETHGET", 6);
						memcpy(tcps_senbuf+6,tcps_recbuf+6,25);		
						send(1,tcps_senbuf,31);		

						//保存参数
						ManSave();						
						//重新配置
						W5500_Config();
					}
				
				}
				
				if(tcp_rec_len == 7)   //SAxxx#
				{
					if(tcps_recbuf[0] == 'S' && tcps_recbuf[1] == 'A' && tcps_recbuf[6] == '#' )
					{
						CH1_Val_Tmp = ( (tcps_recbuf[3]-48)*100+(tcps_recbuf[4]-48)*10+(tcps_recbuf[5]-48));
						if(CH1_Val_Tmp <= 999)
						{
							if(CH1_Val_Tmp!=CH1_pwm)
							{
								CH1_pwm = CH1_Val_Tmp;
								if(CH1_pwm<70 && CH1_pwm>0)
								{
									MCP4922_DAC_All((uint16_t)((MAX_DAC_Value/999.0) * 70.0));
								}
								else
								{
								MCP4922_DAC_All((uint16_t)((MAX_DAC_Value/999.0) * (float)CH1_pwm));
								}
							}
							
							memcpy(tcps_senbuf, "a", 1);
							send(1,tcps_senbuf,1);	
							update_pwm_flag=1;	
						}
											
					}
				}
				
				if(tcp_rec_len == 8)   //SPAxxx#    DLAXXX#  
				{
					if(tcps_recbuf[0] == 'S' && tcps_recbuf[1] == 'P' && tcps_recbuf[2] == 'A'  && tcps_recbuf[7] == '#' )
					{
						CH1_Val_Tmp = ( (tcps_recbuf[3]-48)*1000+(tcps_recbuf[4]-48)*100+(tcps_recbuf[5]-48)*10+(tcps_recbuf[6]-48));
						if(CH1_Val_Tmp<=999)
						{
							CH1_tim = CH1_Val_Tmp;
							memcpy(tcps_senbuf, "pa", 2);
							send(1,tcps_senbuf,2);
							FPGA_UPDATE_cnt=11;						
							update_pwm_flag=1;
						}
					
												
					}
					if(tcps_recbuf[0] == 'D' && tcps_recbuf[1] == 'L' && tcps_recbuf[2] == 'A'  && tcps_recbuf[7] == '#' )
					{
						CH1_Val_Tmp = ( (tcps_recbuf[3]-48)*1000+(tcps_recbuf[4]-48)*100+(tcps_recbuf[5]-48)*10+(tcps_recbuf[6]-48));
						if(CH1_Val_Tmp<=999)
						{
							CH1_DL = CH1_Val_Tmp;
							memcpy(tcps_senbuf, "dla", 3);
							send(1,tcps_senbuf,3);
							FPGA_UPDATE_cnt=11;						
							update_pwm_flag=1;
						}
					
												
					}
					
				}
				
				
				if(tcp_rec_len == 4)    //SPA#   DLA#
				{
					if(tcps_recbuf[0] == 'S' && tcps_recbuf[1] == 'P' && tcps_recbuf[2] == 'A' && tcps_recbuf[3] == '#' )
					{				
						memcpy(tcps_senbuf, "pa", 2);
						tcps_senbuf[2] = '0';
						tcps_senbuf[3] = CH1_tim/100+48;
						tcps_senbuf[4] = CH1_tim/10%10+48;
						tcps_senbuf[5] = CH1_tim%10+48;		
						send(1,tcps_senbuf,6);							
					}
					if(tcps_recbuf[0] == 'D' && tcps_recbuf[1] == 'L' && tcps_recbuf[2] == 'A' && tcps_recbuf[3] == '#' )
					{				
						memcpy(tcps_senbuf, "dla", 3);
						tcps_senbuf[3] = '0';
						tcps_senbuf[4] = CH1_DL/100+48;
						tcps_senbuf[5] = CH1_DL/10%10+48;
						tcps_senbuf[6] = CH1_DL%10+48;		
						send(1,tcps_senbuf,7);							
					}
					
					if(tcps_recbuf[0] == 'T' && tcps_recbuf[1] == 'R' &&  tcps_recbuf[3] == '#' )
					{				
//						memcpy(tcps_senbuf, "dla", 3);
//						tcps_senbuf[3] = '0';
//						tcps_senbuf[4] = CH1_DL/100+48;
//						tcps_senbuf[5] = CH1_DL/10%10+48;
//						tcps_senbuf[6] = CH1_DL%10+48;		
//						send(1,tcps_senbuf,7);			

						CH1_Val_Tmp = tcps_recbuf[2]-48;
						if(CH1_Val_Tmp<4)
						{
							DigOrSP = CH1_Val_Tmp;	
							memcpy(tcps_senbuf, "tr", 2);
							send(1,tcps_senbuf,2);
							change_mode=1;
							Inc_sel_flag=0;
							FPGA_UPDATE_cnt=11;
						}
		
						
					}
					
					
					
				}
				
				
				
				if(tcp_rec_len == 3)    //SA#
				{
					if(tcps_recbuf[0] == 'C' && tcps_recbuf[1] == 'S' && tcps_recbuf[2] == 'T' )
					{				
						memcpy(tcps_senbuf, "CST", 3);
								
						send(1,tcps_senbuf,5);	
					}
					if(tcps_recbuf[0] == 'S' && tcps_recbuf[1] == 'A' && tcps_recbuf[2] == '#' )
					{				
						memcpy(tcps_senbuf, "a0", 2);
						tcps_senbuf[2] = CH1_pwm/100+48;
						tcps_senbuf[3] = CH1_pwm/10%10+48;
						tcps_senbuf[4] = CH1_pwm%10+48;		
						send(1,tcps_senbuf,5);							
					}
					
					if(tcps_recbuf[0] == 'T' && tcps_recbuf[1] == 'H' && tcps_recbuf[2] == '#' )   
					{			
						if(DigOrSP==DP_mode)
						{
							HL_H;
//							memcpy(tcps_senbuf, "h", 1);
							tcps_senbuf[0] = 'h';

							send(1,tcps_senbuf,1);
							HorL = 1;
							SW_update_cnt=5;
						}
					
					}
					
					if(tcps_recbuf[0] == 'T' && tcps_recbuf[1] == 'L' && tcps_recbuf[2] == '#' )
					{			
						if(DigOrSP==DP_mode)
						{
							HL_L;
//							memcpy(tcps_senbuf, "l", 1);
							tcps_senbuf[0] = 'h';
							send(1,tcps_senbuf,1);	
							HorL = 0;
							SW_update_cnt=5;
							
							
							
						}
						
					}
					
					if(tcps_recbuf[0] == 'T' && tcps_recbuf[1] == 'R' && tcps_recbuf[2] == '#' )
					{
						memcpy(tcps_senbuf, "tr", 2);
						tcps_senbuf[2] = DigOrSP+48;		
						send(1,tcps_senbuf,3);	
					
					}
					
					
					
				}
				
				if(tcp_rec_len == 2)
				{
					if(tcps_recbuf[0] == 'T' && tcps_recbuf[1] == '#')
					{
						if(HorL)
						{
							memcpy(tcps_senbuf, "H", 1);
							send(1,tcps_senbuf,1);	
						}
						else
						{
							memcpy(tcps_senbuf, "L", 1);
							send(1,tcps_senbuf,1);	
						}
					}
				}
		
			}
			else
			{
				
			}
			
		break;
		case SOCK_CLOSE_WAIT:						
			close(1);								
//			printf("socket 1 Closed, TCP , port %d\r\n",TCP_PORT);	
		break;
		case SOCK_CLOSED:								
			if(socket(1,Sn_MR_TCP,TCP_PORT,Sn_MR_ND) == 1)		// 
//				printf("socket 1 Opened, TCP , port %d\r\n",TCP_PORT);		
		break;
	}
}

*/













