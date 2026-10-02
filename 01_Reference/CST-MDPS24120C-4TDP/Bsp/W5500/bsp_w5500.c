/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp.c
 *
 * @par dependencies
 * - bsp_flash.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the flash read and write.
 *
 * Processing flow:
 * call directly.
 *
 * @version V1.0 2025-0430 *
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_w5500.h"
#include "wizchip_conf.h"
#include "basic_crc.h"
#include "dhcp.h"
#include "spi.h"
#include "communicat.h"
#include "socket.h"
#include "wiz_interface.h"

#include "app.h"
#include "main.h"
#include "stdbool.h"
#include "app_flash.h"
#include "app_w5500.h"
//******************************** Includes *********************************//

//wiz_NetInfo g_temp_WIZNETINFO = {0};//定义网络参数结构体
//uint8_t udp_rce_flag = 0;	        //udp接收标志 0：没收到UDP数据  1：收到UDP数据
//uint8_t udp_recbuf[UDP_RBUF_SIZE];  //udp接收缓存
//uint16_t udp_rec_len = 0;	        //接收到的数据长度

const uint8_t default_ip[4] = {192, 168, 1, 200};
const uint8_t default_sn[4] = {255, 255, 255, 0};
const uint8_t default_gw[4] = {192, 168, 1, 1};

uint8_t mac[6]	= {0};
uint8_t ip [4] 	= {192,168,1,200}; 
uint8_t sn [4] 	= {255,255,255,0}; 
uint8_t gw [4] 	= {192,168,1,1}; 
uint8_t dns[4]	= {0,0,0,0}; 			//DNS服务器地址
dhcp_mode dhcp = NETINFO_DHCP;			//动静态分配

uint8_t g_dhcp_state = 1;			//DHCP开关标志   0-静态    1-动态
uint8_t dhcpmsgbuf[600];

#define STATE_DHCP_STOP        6
extern uint8_t DHCP_CHADDR[6];

//#define IR_SOCK(ch) (0x01 << ch) /**< check socket interrupt */

//TCP全局变量
//uint8_t tcps_recbuf[TCP_SOCKET_MAX_NUM][TCPS_RBUF_SIZE];//tcp接收缓存
//uint8_t tcps_senbuf[TCPS_SBUF_SIZE];//tcp发送缓存

uint8_t  broadcastip[4]={255,255,255,255};//广播地址
uint8_t  udp_destip[4];//udp电脑端的ip
uint16_t udp_destport;//udp电脑端的端口


volatile uint8_t w5500_sock_int_status[W5500_SOCKET_NUM] = {0};
volatile uint8_t w5500_irq_flag = 0 ;

void W5500_Config(void);
void w5500_send_func(uint8_t sn,uint8_t *buf, uint16_t len);

static bool is_ip_invalid(const uint8_t ip[4]) {
    if ((ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) ||
        (ip[0] == 255 && ip[1] == 255 && ip[2] == 255 && ip[3] == 255)) {
        return true;
    }
    return false;
}

static bool is_same_subnet(const uint8_t ip[4], const uint8_t gw[4], const uint8_t sn[4]) 
{
    for (int i = 0; i < 4; i++) 
	{
        if ((ip[i] & sn[i]) != (gw[i] & sn[i])) 
		{
            return false;
        }
    }
    return true;
}

// 检查网络参数合法性
bool check_net_params(const uint8_t ip[4], const uint8_t sn[4], const uint8_t gw[4]) 
{
    if (is_ip_invalid(ip) || is_ip_invalid(sn) || is_ip_invalid(gw)) {
        return false;
    }
    if (!is_same_subnet(ip, gw, sn)) {
        return false;
    }
    if (memcmp(ip, gw, 4) == 0) {
        return false;
    }
    return true;
}



void bsp_save_new_ip(void)
{
	if (!check_net_params(ip, sn, gw)) {
        return;
    }
	uint64_t TEMP_buff1[3];
	TEMP_buff1[0] = (((uint64_t)FLASH_CHECK_NUMMBER_1>>0))+(((uint64_t)FLASH_CHECK_NUMMBER_2<<8));
	TEMP_buff1[1] = ((ip[0]>>0) )				|	(((uint64_t)ip[1]<<8) ) |								
					(((uint64_t)ip[2]<<16) )	|	(((uint64_t)ip[3]<<24) )|   
					(((uint64_t)sn[0]<<32) )	|	(((uint64_t)sn[1]<<40) )|
					(((uint64_t)sn[2]<<48) )	|	(((uint64_t)sn[3]<<56));
	TEMP_buff1[2] = ((gw[0]<<0) ) 				|	(((uint64_t)gw[1]<<8) ) |
					(((uint64_t)gw[2]<<16) )	|	(((uint64_t)gw[3]<<24) )|  
					(((uint64_t)g_dhcp_state<<32) );		

	uint64_t name[3] = {0};
	if (strlen((char *)gDeviceHWConfigS.szModulename) > 21) {
		gDeviceHWConfigS.szModulename[20] = '\0'; 
	}
	memcpy(name, gDeviceHWConfigS.szModulename, 21);
    
	bsp_FlashErase(W5500_FLASH_ADDR);
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,W5500_FLASH_ADDR,TEMP_buff1[0]);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,W5500_FLASH_ADDR+8,TEMP_buff1[1]);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,W5500_FLASH_ADDR+16,TEMP_buff1[2]);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,W5500_FLASH_ADDR+24,name[0]);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,W5500_FLASH_ADDR+32,name[1]);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,W5500_FLASH_ADDR+40,name[2]);
	HAL_FLASH_Lock(); 
}


void bsp_read_ip_config(void )
{
	if (!check_net_params(ip, sn, gw)) {
        return;
    }

	ch9121_config_init();
	if( (FLASH_CHECK_NUMMBER_1 == (*(uint8_t *)(W5500_FLASH_ADDR)))  &&
			(FLASH_CHECK_NUMMBER_2 == (*(uint8_t *)(W5500_FLASH_ADDR+1))))
	{
		for(uint8_t i = 0; i < 4; i++)
		{
			ip[i] = (*(uint8_t *)(W5500_FLASH_ADDR+8+i));
			sn[i] = (*(uint8_t *)(W5500_FLASH_ADDR+12+i));
			gw[i] = (*(uint8_t *)(W5500_FLASH_ADDR+16+i));
		}
		g_dhcp_state = (*(uint8_t *)(W5500_FLASH_ADDR+20));

		for (int i = 0; i < 21; i++) {
			gDeviceHWConfigS.szModulename[i] = *((uint8_t *)(W5500_FLASH_ADDR + 24 + i));
		}

		if(g_dhcp_state != 0xff)
		{
			if(g_dhcp_state == DHCP_ON){
				dhcp = NETINFO_DHCP;
			}
			else{
				dhcp = NETINFO_STATIC;
			}
		}
		else
		{
			g_dhcp_state = DHCP_ON;
			dhcp = NETINFO_DHCP;
		}
	}
	else
	{
		g_dhcp_state = DHCP_ON;
		dhcp = NETINFO_DHCP;
	}
}



void UIDtoMac(void)
{
	uint8_t UID[12] = {0};
	uint32_t base_addr = 0x1FFF7590;
	for (int i = 0; i < 12; i++) {
        UID[11 - i] = *((uint8_t *)(base_addr + i));
    }

	mac[0] = 0x00;	
	mac[1] = (crc16((uint8_t*)(0x1FFF7590), 12)>>8)&0x00FF;//对UID取正序CRC校验
	mac[2] = (crc16((uint8_t*)(0x1FFF7590), 12))&0x00FF;//对UID取正序CRC校验
	mac[3] = (crc16(UID, 12)>>8)&0x00FF;
	mac[4] = (crc16(UID, 12))&0x00FF;
	mac[5] = UID[0]^UID[1]^UID[2]^UID[3]^UID[4]^UID[5]^UID[6]^UID[7]^UID[8]^UID[9]^UID[10]^UID[11];//BCC校验
    memcpy(DHCP_CHADDR, mac, 6);
}
	

/*******************************************************
 * @ brief Call back for ip assing & ip update from DHCP
 *******************************************************/
void my_ip_assign(void )
{
   getIPfromDHCP(ip);
   getGWfromDHCP(gw);
   getSNfromDHCP(sn);
   getDNSfromDHCP(dns);
   dhcp = NETINFO_DHCP;

   /* Network initialization */
	W5500_Config();
	bsp_save_new_ip();

#ifdef _MAIN_DEBUG_
   //Display_Net_Conf();
   printf("DHCP LEASED TIME : %ld Sec.\r\n", getDHCPLeasetime());
   printf("\r\n");
#endif
}

/************************************
 * @ brief Call back for ip Conflict
 ************************************/
void my_ip_conflict(void)
{
#ifdef _MAIN_DEBUG_
	printf("CONFLICT IP from DHCP\r\n");
#endif
   //halt or reset or any...
   while(1); // this example is halt.
}



//W5500网络配置
void W5500_Config(void)
{
	if (!check_net_params(ip, sn, gw)) {
		memcpy(ip, default_ip, sizeof(ip));
		memcpy(sn, default_sn, sizeof(sn));
		memcpy(gw, default_gw, sizeof(gw));
    }
	wiz_NetInfo gWIZNETINFO = {0};
	wiz_NetTimeout w_NetTimeout;

	UIDtoMac();
	if(ip[0] == 0)
	{
		memcpy(ip, default_ip, sizeof(ip));
	}
	// Assign network parameter structure
	memcpy(gWIZNETINFO.ip, ip, 4);
	memcpy(gWIZNETINFO.sn, sn, 4);
	memcpy(gWIZNETINFO.gw, gw, 4);
	memcpy(gWIZNETINFO.mac, mac,6);
	memcpy(gWIZNETINFO.dns,dns,4);
	gWIZNETINFO.dhcp = dhcp; //
	
	// Register callback functions for IP assignment and conflict detection
	reg_dhcp_cbfunc(0, 0, 0);

	if(gWIZNETINFO.dhcp == NETINFO_STATIC)
	{
		DHCP_stop();
		// Configure network parameters
		ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);		
		ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);	
		//ctlwizchip(CW_GET_ID,(void*)chipid); 
	}
	else
	{
		// Configure network parameters
		ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
		//DHCP_init(2,dhcpmsgbuf);	//初始化dhcp
		DHCP_init(DHCP_CONNET_NUMBER,dhcpmsgbuf);	//初始化dhcp
	}
	//初始化socket发送缓存大小，NULL时默认为2K
	wizchip_init(NULL, NULL);
	
	//设置重试时间和次数
	w_NetTimeout.retry_cnt = 10;
	w_NetTimeout.time_100us = 3000;
	wizchip_settimeout(&w_NetTimeout);

	//开启呼吸包机制
	//IINCHIP_WRITE(Sn_KPALVTR(1),0x04);// KEEP ALIVE IS 4*5s
	IINCHIP_WRITE(Sn_KPALVTR(1),0x0A);// KEEP ALIVE IS 10*5s

}


void W5500_DHCP_Config(void)
{
	//DHCP_init(2,dhcpmsgbuf);	//初始化dhcp
	reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
}
	
//W5500片选
void W5500_CS_Select(void)
{
	HAL_GPIO_WritePin(NET_SCS_GPIO_Port, NET_SCS_Pin, GPIO_PIN_RESET);//置W5500的SCS为低电平
}

//W5500取消片选
void W5500_CS_Deselect(void)
{
	HAL_GPIO_WritePin(NET_SCS_GPIO_Port, NET_SCS_Pin, GPIO_PIN_SET); //置W5500的SCS为高电平
}

////W5500写一个字节
void W5500_WriteByte(uint8_t byte)
{
#if SPI_DMA
	uint8_t tx_byte = byte;
	if(HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY)while(1);
	HAL_SPI_Transmit_DMA(&hspi2, &tx_byte, 1);
#else
	//HAL_SPI_Transmit(&hspi2, &byte, 1, 3);
	while(!LL_SPI_IsActiveFlag_TXE(SPI2));
    LL_SPI_TransmitData8(SPI2, byte);
    // 等待发送完成
    while(!LL_SPI_IsActiveFlag_RXNE(SPI2));
    (void)LL_SPI_ReceiveData8(SPI2); // 读走数据，避免溢出
#endif
}

////W5500读一个字节
uint8_t W5500_ReadByte(void)
{
#if SPI_DMA
	uint8_t rx_byte = 0;
	HAL_SPI_Receive_DMA(&hspi2, &rx_byte, 1);
	if(HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY)while(1);
  	return rx_byte;
#else
	//uint8_t i;
	//HAL_SPI_Receive(&hspi2, &i, 1, 3);	
	//return i;
	while(!LL_SPI_IsActiveFlag_TXE(SPI2));
    LL_SPI_TransmitData8(SPI2, 0xFF);
    // 等待接收完成
    while(!LL_SPI_IsActiveFlag_RXNE(SPI2));
    return LL_SPI_ReceiveData8(SPI2);
#endif
}

void wizchip_write_buff(uint8_t *buf, uint16_t len)
{
    uint16_t idx = 0;
    for (idx = 0; idx < len; idx++)
    {
        W5500_WriteByte(buf[idx]);
    }
}

/**
 * @brief   SPI read buff from wizchip
// ... existing code ...
 */
void wizchip_read_buff(uint8_t *buf, uint16_t len)
{
    uint16_t idx = 0;
    for (idx = 0; idx < len; idx++)
    {
        buf[idx] = W5500_ReadByte();
    }
}

//进入临界区
void critical_enter(void)
{
//__set_PRIMASK(1);
//__disable_irq();
}

//退出临界区
void critical_exit(void)
{
//__set_PRIMASK(0);
//__enable_irq();
}

//W5500初始化
void W5500_Init(void)
{
	uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2, 2, 2 } };
	HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);	//复位引脚拉高
	//reg_wizchip_cris_cbfunc(critical_enter, critical_exit); 			// 注册临界区函数
	reg_wizchip_cs_cbfunc(W5500_CS_Select, W5500_CS_Deselect);			// 注册片选函数
	reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte); 			// 注册读写函数
	reg_wizchip_spiburst_cbfunc(wizchip_read_buff, wizchip_write_buff); // 注册读写函数
	/* wizchip initialize*/ 
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1) {
		//printf("WIZCHIP Initialized fail.\r\n");
		while (1);
	}
	for(int i=0xFFFF; i>0; i--); // 短暂延时,不然导致初始化失败	
    HAL_Delay(100);   
}


/**
 * @brief w5500 init .
 * 
 * hardware init w5500  and configed to DHCP
 * 
 * 
 * Steps:
 *  1. Adds before while(1) of main.
 * 
 * @return led_handler_status_t : Status of the function.
 * 
 * */
bsp_w5500_status_t bsp_net_w5500_init(void)
{
	wizchip_status_t result ;  
	W5500_Init();
	W5500_Config();		/* DHCP inited */
	
	#if DEBUG_ENABLE 
		result = wizchip_init_check(1);
		if( WIZCHIP_OK == result )
			return BSP_W5500_OK;
		else	
			return BSP_W5500_ERROR;
	#else
		result = wizchip_init_check(0);
		if( WIZCHIP_OK == result )
			return BSP_W5500_OK;
		else	
			return BSP_W5500_ERROR;
	#endif
}

void w5500_send_func(uint8_t sn,uint8_t *buf, uint16_t len)
{
    send(sn, buf, len); // 1为socket号，按实际情况调整
}


void bsp_w5500_init(void)
{
	bsp_read_ip_config();
	if(BSP_W5500_ERROR == bsp_net_w5500_init())
	{
		Error_Handler();
	}

}


