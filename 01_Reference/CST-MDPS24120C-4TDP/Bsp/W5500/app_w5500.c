//******************************** Includes *********************************//
#include "bsp_w5500.h"
#include "stdio.h"
#include "string.h"
#include "wizchip_conf.h"
#include "basic_crc.h"
#include "dhcp.h"
#include "spi.h"
//#include "bsp_flash.h"
#include "communicat.h"
#include "socket.h"
//#include "bsp.h"
#include "wiz_interface.h"

#include "app.h"
#include "main.h"
#include "stdbool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app_flash.h"
#include "app_w5500.h"
#include "protocol_public.h"
#include "app_router.h"
//******************************** Includes *********************************//


uint8_t udp_rce_flag = 0;	        //udp接收标志 0：没收到UDP数据  1：收到UDP数据
uint8_t udp_recbuf[UDP_RBUF_SIZE];  //udp接收缓存
uint16_t udp_rec_len = 0;	        //接收到的数据长度

//uint8_t tcp_rce_flag = 0;	        //tcp接收标志 0：没收到数据  1：收到数据
uint8_t tcps_recbuf[TCP_SOCKET_MAX_NUM][TCPS_RBUF_SIZE];//tcp接收缓存
//uint8_t tcps_senbuf[TCPS_SBUF_SIZE];//tcp发送缓存
uint8_t temp_tcps_recbuf[TCP_SOCKET_MAX_NUM][TCPS_RBUF_SIZE];//tcp接收临时缓存

/*extern uint8_t mac[6];//MAC地址
extern uint8_t ip[4]; //IP地址
extern uint8_t sn[4]; //子网掩码
extern uint8_t gw[4]; //网关
extern uint8_t dns[4]; 	//DNS服务器地址
extern dhcp_mode dhcp;		//动静态分配
extern uint8_t  broadcastip[4];//广播地址
extern uint8_t  udp_destip[4];//udp电脑端的ip
extern uint16_t udp_destport;//udp电脑端的端口*/

unsigned char *tcp_tx_buffer;
unsigned int net_tx_length = 0;

DeviceHWConfigS gDeviceHWConfigS;

wiz_NetInfo udp_conf_info;
/**
 * @brief app of UDP handle. 
 * @note   upd search and net parament config
 * @return  No
 * */
#if  OPT_W5500
void app_upd_handle(uint8_t s_sn)
{	
	if(8 < sn)return;	
	uint8_t uc_id_check_num = 0;
	uint8_t uc_bcc_check	= 0;
	uint8_t temp_udp_sendbuf[30] = { 0 };

	//wiz_NetInfo udp_conf_info;
	//wiz_NetInfo gWIZNETINFO;//定义网络参数结构体
	switch(getSn_SR(s_sn))															
	{
		case SOCK_UDP:												   			
			i f((getSn_RX_RSR(s_sn))>0 && udp_rce_flag == 0)
			{ 
				if( (udp_rec_len = recvfrom(s_sn,udp_recbuf, UDP_RBUF_SIZE, udp_destip,&udp_destport)) > 0)
				{			
					udp_rce_flag = 1;
				}		
			}
		break;
		/*	Socket is released	*/
		case SOCK_CLOSED:														
			if (socket(s_sn,Sn_MR_UDP,UDP_PORT,0) < 0){
				//printf("socket 0 UDP opened fail\r\n");
			}
		break;
		default:
			break;
	} 
	wizchip_getnetinfo(&udp_conf_info);
		
	//接收了UDP数据  
	if(udp_rce_flag == 1)
	{
		
		//ETHGET
		if(udp_rec_len == 3)
		{
			if(udp_recbuf[0] == 'O' && udp_recbuf[1] == 'P' && udp_recbuf[2] == 'T' )
			{			
                /* Data: |4f 50 54|60 24 |a2 45 23 01 12 |010108010000 */
                /*	OPT	*/
                memcpy(temp_udp_sendbuf, OPT_NAME, 3);
                /* 型号	*/
                temp_udp_sendbuf[3] = OPT_TYPE_FIRST;
                temp_udp_sendbuf[4] = OPT_TYPE_SECOND;
                /* ID号 */
                memcpy(temp_udp_sendbuf+5, udp_conf_info.mac + 1, 5);
                /* 版本号 */
                temp_udp_sendbuf[10] = OPT_VERSION_FIRST;
                temp_udp_sendbuf[11] = OPT_VERSION_SECOND;
                /* 通道数 */
                temp_udp_sendbuf[12] = OPT_CHANNEL_NUM;
                /* 返回值 */
                //temp_udp_sendbuf[13] = OPT_UDP_BACK;
				temp_udp_sendbuf[13] = 0x00;
                /* 检验字 */
                temp_udp_sendbuf[14] = OPT_UDP_RADIO_CHECK;
                /* DHCP  配置 */
                if( NETINFO_DHCP == udp_conf_info.dhcp )
                {
                    temp_udp_sendbuf[15] = 0x01;
                }
                else
                {
                    temp_udp_sendbuf[15] = 0x00;            
                }
                sendto(s_sn,temp_udp_sendbuf,16, broadcastip, udp_destport);
			}
		}

    /*************** 获取网络配置 **********************/
		if( udp_rec_len == OPT_GET_NET_CONFIG_DATA_LENGTH )
		{
			if((udp_recbuf[0] == OPT_FF_CMD ) && ( udp_recbuf[1] == OPT_FF_READ_PROPERTY ) )
			{
				//BCC sum
				for (uint8_t i = 0; i < OPT_GET_NET_CONFIG_DATA_LENGTH - 1; i++)
				{
					uc_bcc_check ^=  udp_recbuf[i];
				}
				//check BCC 
				if( uc_bcc_check == udp_recbuf[10])
				{
					for(uint8_t i = 0 ; i < 5 ; i++)
					{
						if( udp_conf_info.mac[i+1] == udp_recbuf[i+2])
						{
							uc_id_check_num++;
						}
					}

					if(uc_id_check_num == 5)
					{
                        uc_id_check_num = 0;
						/* Data: |4f 50 54 |60 24 |a2 45 23 01 12 |01 01 |08 | 01 | 00 | 00 |ac 10 32 14| ff  ff ff 00 |00 00 00 00 | */
						/*			OPT															IP				gw			sn		  */
						memset(temp_udp_sendbuf, 0, 20);
						
						memcpy(temp_udp_sendbuf, OPT_NAME, 3);
						/* 型号	*/
						temp_udp_sendbuf[3] = OPT_TYPE_FIRST;
						temp_udp_sendbuf[4] = OPT_TYPE_SECOND;
						/* ID号 */
						memcpy(temp_udp_sendbuf+5, udp_conf_info.mac + 1, 5);
						/* 版本号 */
						temp_udp_sendbuf[10] = OPT_VERSION_FIRST;
						temp_udp_sendbuf[11] = OPT_VERSION_SECOND;
						/* 通道数 */
						temp_udp_sendbuf[12] = OPT_CHANNEL_NUM;
						/* 返回值 */
						//temp_udp_sendbuf[13] = OPT_UDP_BACK;
						temp_udp_sendbuf[13] = 0x00;
						/* 检验字 */
						temp_udp_sendbuf[14] = OPT_UDP_RADIO_CHECK;
						/* 动静态IP分配 */
                        /* 01 ： DHCP        */
                        /* 00 :  STATIC     */
                        if( NETINFO_DHCP == udp_conf_info.dhcp )		//NETINFO_DHCP = 2
                        {
                            temp_udp_sendbuf[15] = 0x01;
                        }
                        else							// NETINFO_STATIC = 1
                        {
                            temp_udp_sendbuf[15] = 0x00;   
                        }

						memcpy(temp_udp_sendbuf+16, udp_conf_info.ip, 4);
                        memcpy(temp_udp_sendbuf+20, udp_conf_info.sn, 4);
                        memcpy(temp_udp_sendbuf+24, udp_conf_info.gw, 4);
						sendto(s_sn,temp_udp_sendbuf,28, broadcastip, udp_destport);
					}
                    /* 错误返回 F0 00*/
                    else
                    {
                        temp_udp_sendbuf[0] =  0xF0 ;
                        temp_udp_sendbuf[1] =  0x00 ;
                        sendto(s_sn,temp_udp_sendbuf,2, broadcastip, udp_destport);
                    }
				}
				else  /* Check bcc */
        		{
					temp_udp_sendbuf[0] =  0xF0 ;
					temp_udp_sendbuf[1] =  0x00 ;
					
					sendto(s_sn,temp_udp_sendbuf,2, broadcastip, udp_destport);
        		}
			}
		}

    /*********************** 修改网络配置 **************************/
		if ( udp_rec_len == OPT_SET_NET_CONFIG_DATA_LENGTH ) //21
		{
			if( (udp_recbuf[0] == OPT_FD_CMD ) && ( udp_recbuf[1] == OPT_FD_READ_PROPERTY ) )
			{
				/*   没开DHCP Data: |fd 01| c5 b3 8a 22 2f | 00 | c0 a8 01 bd | ff ff ff 00 | 00 00 00 00 | 26   */
                /*   开启DHCP Data: |fd 01| c5 b3 8a 22 2f | 01 | c0 a8 01 bc | 00 00 00 00 | 00 00 00 00 | d9   */

				for (uint8_t i = 0; i < OPT_SET_NET_CONFIG_DATA_LENGTH - 1; i++)
				{
					uc_bcc_check ^=  udp_recbuf[i];
				}

				if( uc_bcc_check == udp_recbuf[OPT_SET_NET_CONFIG_DATA_LENGTH - 1])
				{
					for(uint8_t i = 0 ; i < 5 ; i++)
					{
						if( mac[i+1] == udp_recbuf[i+2])
						{
							uc_id_check_num++;
						}
					}

					if( 5 == uc_id_check_num )
					{
             			uc_id_check_num = 0;
						/*  开启DHCP   */
						if(1 == udp_recbuf[7])
						{
							dhcp = NETINFO_DHCP;
										
							g_dhcp_state = DHCP_ON;
							//memcpy(ip, udp_recbuf + 8, 4);
							//memcpy(sn, udp_recbuf + 12, 4);
							//memcpy(gw, udp_recbuf + 16, 4);	
							bsp_save_new_ip();
							W5500_Config();		
						}
						/*  静态IP */
						else
						{
							dhcp = NETINFO_STATIC;
							
							memcpy(ip, udp_recbuf + 8, 4);
							memcpy(sn, udp_recbuf + 12, 4);
							memcpy(gw, udp_recbuf + 16, 4);
							
							if( ip[0] == 0 || ip[1] == 0 || ip[2] == 0 ||
								sn[0] == 0 || sn[1] == 0 || sn[2] == 0 ||
								gw[0] == 0 || gw[1] == 0 || gw[2] == 0){
								memcpy(ip, udp_conf_info.ip, 4);
                        		memcpy(sn, udp_conf_info.sn, 4);
                        		memcpy(gw, udp_conf_info.gw, 4);
								//ip[0] = 192;ip[1] = 168;ip[2] = 1;ip[3] = 200;
							}
							g_dhcp_state = DHCP_OFF;
							bsp_save_new_ip();
							W5500_Config();
						}
						/* 成功收到后回复 */
						temp_udp_sendbuf[0] =  0xE0 ;
						temp_udp_sendbuf[1] =  0x00 ;
						sendto(s_sn,temp_udp_sendbuf,2, broadcastip, udp_destport);
					}
					else
					{
						temp_udp_sendbuf[0] =  0xF0 ;
						temp_udp_sendbuf[1] =  0x00 ;
						sendto(s_sn,temp_udp_sendbuf,2, broadcastip, udp_destport);
					}
				}
				else
				{
					temp_udp_sendbuf[0] =  0xF0 ;
					temp_udp_sendbuf[1] =  0x00 ;
					sendto(s_sn,temp_udp_sendbuf,2, broadcastip, udp_destport);
				}	
			}
		}
		
		memset(udp_recbuf,0,UDP_RBUF_SIZE);
//		memset(udp_senbuf,0,UDP_SBUF_SIZE);
		udp_rce_flag = 0;
	}
}
#elif CST_W5500
void ch9121_config_init(void )
{
	gDeviceHWConfigS.bDevType 			= 0x21; 
	gDeviceHWConfigS.bAuxDevType 		= 0x21; 
	gDeviceHWConfigS.bIndex 			= 0x01;
	gDeviceHWConfigS.bDevHardwareVer 	= 0x02;	
	gDeviceHWConfigS.bDevSoftwareVer 	= 0x2F; 
	memcpy(gDeviceHWConfigS.szModulename, CST_NAME , sizeof(gDeviceHWConfigS.szModulename));
}


uint8_t temp_udp_sendbuf[UDP_SBUF_SIZE] = { 0 };

void app_upd_handle(uint8_t s_sn)
{	
	if(8 < s_sn)return;	
	uint8_t name_len = 0;

	name_len = strlen((char *)gDeviceHWConfigS.szModulename) + 1;
	switch(getSn_SR(s_sn))															
	{
		case SOCK_UDP:												   			
			if((getSn_RX_RSR(s_sn))>0 && udp_rce_flag == 0){ 
				if( (udp_rec_len = recvfrom(s_sn,udp_recbuf, UDP_RBUF_SIZE, udp_destip,&udp_destport)) > 0){			
					udp_rce_flag = 1;
				}		
			}
		break;
		case SOCK_CLOSED:														
			if (socket(s_sn,Sn_MR_UDP,UDP_PORT,0) < 0){
				//printf("socket 0 UDP opened fail\r\n");
			}
		break;
		default:
			break;
	} 
	wizchip_getnetinfo(&udp_conf_info);
		
	//接收了UDP数据  
	if(udp_rce_flag == 1)
	{
		if(udp_rec_len == 285)
		{
			if(memcmp(udp_recbuf, CH9121_FLAG, CH9121_FLAG_LEN) == 0)
			{		
				//搜索设备		
				if(udp_recbuf[16] == NET_MODULE_CMD_SEARCH)
				{
					memcpy(temp_udp_sendbuf, CH9121_FLAG, CH9121_FLAG_LEN);
					temp_udp_sendbuf[16] = NET_MODULE_ACK_SEARCH;
					memcpy(temp_udp_sendbuf+17, &udp_conf_info.mac, 6);
					temp_udp_sendbuf[29] = name_len + 4;
					memcpy(temp_udp_sendbuf+30, &udp_conf_info.ip, 4);
					memcpy(temp_udp_sendbuf+34, gDeviceHWConfigS.szModulename, name_len);
					temp_udp_sendbuf[34+name_len] = gDeviceHWConfigS.bDevSoftwareVer;
					sendto(s_sn,temp_udp_sendbuf,285, broadcastip, udp_destport);
				}
				//获取设备信息
				if(udp_recbuf[16] == NET_MODULE_CMD_GET)
				{
					if(memcmp(udp_recbuf+17, &udp_conf_info.mac, 6) == 0)
					{
						memset(temp_udp_sendbuf, 0, sizeof(temp_udp_sendbuf));
						memcpy(temp_udp_sendbuf, CH9121_FLAG, CH9121_FLAG_LEN);
						temp_udp_sendbuf[16] = NET_MODULE_ACK_GET;
						memcpy(temp_udp_sendbuf+17, &udp_conf_info.mac, 6);
						temp_udp_sendbuf[29] = 0xcc;
						temp_udp_sendbuf[30] = gDeviceHWConfigS.bDevType;
						temp_udp_sendbuf[31] = gDeviceHWConfigS.bAuxDevType;
						temp_udp_sendbuf[32] = gDeviceHWConfigS.bIndex;
						temp_udp_sendbuf[33] = gDeviceHWConfigS.bDevHardwareVer;
						temp_udp_sendbuf[34] = gDeviceHWConfigS.bDevSoftwareVer;
						memcpy(temp_udp_sendbuf+35, gDeviceHWConfigS.szModulename, name_len);
						memcpy(temp_udp_sendbuf+56, &udp_conf_info.mac, 6);
						memcpy(temp_udp_sendbuf+62, &udp_conf_info.ip, 4);
						memcpy(temp_udp_sendbuf+66, &udp_conf_info.gw, 4);
						memcpy(temp_udp_sendbuf+70, &udp_conf_info.sn, 4);
						temp_udp_sendbuf[74] = (udp_conf_info.dhcp == NETINFO_DHCP) ? 0x01 : 0x00;
						temp_udp_sendbuf[75] = 0x50; //web net address
						temp_udp_sendbuf[94] = 0XFF;

        				static const uint8_t udp_mode_blk[] = {0x02, 0x01, 0xb8, 0x0b};
        				memcpy(temp_udp_sendbuf+106, udp_mode_blk, sizeof(udp_mode_blk));
						memcpy(temp_udp_sendbuf+110, &udp_destip, 4);
						static const uint8_t blk114[] = {0xd0, 0x07, 0x00, 0x4b};
        				memcpy(temp_udp_sendbuf+114, blk114, sizeof(blk114));
						static const uint8_t blk120[] = {0x08, 0x01, 0x04, 0x01};
						memcpy(temp_udp_sendbuf+120, blk120, sizeof(blk120));
						temp_udp_sendbuf[125] = 0x04;

						temp_udp_sendbuf[169] = 0x01;temp_udp_sendbuf[170] = 0x01;
						temp_udp_sendbuf[173] = 0xc8;temp_udp_sendbuf[174] = 0x19;
						memcpy(temp_udp_sendbuf+175, &udp_destip, 4);
						temp_udp_sendbuf[179] = 0xe8;temp_udp_sendbuf[180] = 0x03;
						temp_udp_sendbuf[182] = 0x4b;
        				static const uint8_t blk185[] = {0x08, 0x01, 0x04, 0x01};
        				memcpy(temp_udp_sendbuf+185, blk185, sizeof(blk185));
						temp_udp_sendbuf[190] = 0x04;
						sendto(s_sn,temp_udp_sendbuf,285, broadcastip, udp_destport);
					}
				}
				
				if(udp_recbuf[16] == NET_MODULE_CMD_SET)
				{
					uint8_t devname[21]	= {0};
					if(memcmp(udp_recbuf+17, &udp_conf_info.mac, 6) == 0)
					{
						//get new config
						memset(gDeviceHWConfigS.szModulename,0,sizeof(gDeviceHWConfigS.szModulename));
						memcpy(devname, udp_recbuf+35, 21);
						memcpy(gDeviceHWConfigS.szModulename, devname, strlen((const char *)devname));
						memcpy(ip, udp_recbuf+62, 4);
						memcpy(gw, udp_recbuf+66, 4);
						memcpy(sn, udp_recbuf+70, 4);
						dhcp = (udp_recbuf[74] == 0x01)?  NETINFO_DHCP : NETINFO_STATIC;
						if(dhcp == NETINFO_STATIC){
							g_dhcp_state = DHCP_OFF;
						}else{
							g_dhcp_state = DHCP_ON;
						}
						//resend 
						memcpy(temp_udp_sendbuf, CH9121_FLAG, CH9121_FLAG_LEN);
						temp_udp_sendbuf[16] = NET_MODULE_ACK_SET;
						memcpy(temp_udp_sendbuf+17, udp_recbuf+17, 6);
						memcpy(temp_udp_sendbuf+23, udp_recbuf+23, 6);
						temp_udp_sendbuf[29] = 0x00;
						memcpy(temp_udp_sendbuf+30, udp_recbuf+30, 255);
						sendto(s_sn,temp_udp_sendbuf,285, broadcastip, udp_destport);
						bsp_save_new_ip();
						W5500_Config();

					}	
				}	
				memset(udp_recbuf,0,UDP_RBUF_SIZE);
				memset(temp_udp_sendbuf,0,UDP_SBUF_SIZE);
				udp_rce_flag = 0;	
			}
		}
	}
}
#endif


void app_udp_dhcp_handle(uint8_t s_sn)
{
	app_upd_handle(s_sn);

	if(dhcp == NETINFO_DHCP)
	{
		DHCP_run();
	}
}


/**
 * @brief app of tcp socket_1  handle 
 * 
 * 
 * @return No.
 * 
 * */
uint8_t uc_conneted[8] = {0};

void app_tcp_handle(uint8_t sn)							
{
	if( TCP_SOCKET_MAX_NUM <= sn )return;
    uint16_t tcp_rec_len = 0;
    //static uint8_t uc_conneted[8] = 0;
	int len = 0;
	
	switch(getSn_SR(sn))								// Socket SR_register access 
	{
		case SOCK_INIT:									
			listen(sn);									//Listen to a connection request from a client.
		break;

		case SOCK_ESTABLISHED:							// Socket Success to connect
			len = getSn_RX_RSR(sn);						
			if(len > 0)
			{		
			
				tcp_rec_len = recv(sn,tcps_recbuf[sn],TCPS_RBUF_SIZE);
				memcpy(temp_tcps_recbuf[sn], tcps_recbuf[sn], tcp_rec_len);	
				#if APP_ROUTER_ENABLE
					//router_process_tcp(sn, tcps_recbuf[sn], tcp_rec_len);
					router_process_tcp(sn, temp_tcps_recbuf[sn], tcp_rec_len);
				#else
					//protocol_parse_entry(tcps_recbuf[sn],tcp_rec_len,w5500_send_func,sn);
					input_data(tcps_recbuf[sn],tcp_rec_len);
					net_tx_length = 0;
					analysis_command();
					tcp_tx_buffer = get_prepare_tx_buffer(&net_tx_length);
					send(sn,tcp_tx_buffer,net_tx_length);
				#endif
			}
		break;

		case SOCK_CLOSE_WAIT:						
			close(sn);								
		break;

		case SOCK_CLOSED:
            uc_conneted[sn] = 0;
			if(socket(sn,Sn_MR_TCP,TCP_PORT,Sn_MR_ND) == 1)	
			//printf("socket 1 Opened, TCP , port %d\r\n",TCP_PORT);		
			break;

		default:
			break;
	}

}


