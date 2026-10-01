#ifndef __BSP_W5500_H
#define __BSP_W5500_H
//******************************** Includes *********************************//
#include "main.h"
#include "stdio.h"
#include "stdint.h"
#include "wizchip_conf.h"
//******************************** Includes *********************************//


//******************************** Declaring ********************************//


#define W5500_SOCKET_NUM        8
#define CST_W5500               1
#define OPT_W5500               0
#define SPI_DMA                 0

/*GPIO端口*/
#define NET_SCS_GPIO_Port		    NET_CS_GPIO_Port
#define NET_SCS_Pin					    NET_CS_Pin 
//#define NET_RST_GPIO_Port		SPI1_RESET_GPIO_Port
//#define NET_RST_Pin					SPI1_RESET_Pin

#define TCP_SOCKET_MAX_NUM		  8

#if OPT_W5500
  #define UDP_PORT		6144	//UDP端口号
  #define TCP_PORT		8000	//TCP端口号
#elif CST_W5500       
  #define UDP_PORT		          50000	//UDP port
  #define TCP_PORT		          6600	//TCP port
#endif

#define UDP_RBUF_SIZE	          290
#define UDP_SBUF_SIZE	          290
#define TCPS_RBUF_SIZE          50//100
#define TCPS_SBUF_SIZE	        50//100

#define NET_MODULE_DATA_LENGTH  255 

#define FLASH_CHECK_NUMMBER_1 	0x20
#define FLASH_CHECK_NUMMBER_2   0x25

//modify the number can connet more in dhcp
#define DHCP_CONNET_NUMBER      3

extern uint8_t g_dhcp_state;			//DHCP开关标志   0-静态    1-动态
extern uint8_t mac[6];//MAC地址
extern uint8_t ip[4]; //IP地址
extern uint8_t sn[4]; //子网掩码
extern uint8_t gw[4]; //网关
extern uint8_t dns[4]; 	//DNS服务器地址
extern dhcp_mode dhcp;		//动静态分配
extern uint8_t  broadcastip[4];//广播地址
extern uint8_t  udp_destip[4];//udp电脑端的ip
extern uint16_t udp_destport;//udp电脑端的端口

typedef struct NET_COMM {
unsigned char flag[16]; //通信标识
unsigned char cmd; //命令头
unsigned char id[6]; //CH9121MAC 地址
unsigned char pcid[6]; //PC 的 MAC 地址
unsigned char len; //数据区长度
unsigned char data[NET_MODULE_DATA_LENGTH]; //数据区缓冲区
}net_comm;

typedef enum
{
  BSP_W5500_OK                = 0,           /* Operation completed successfully.  */
  BSP_W5500_ERROR             = 1,           /* Run-time error without case matched*/
  BSP_W5500_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  BSP_W5500_ERRORRESOURCE     = 3,           /* Resource not available.            */
  BSP_W5500_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  BSP_W5500_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  BSP_W5500_ERRORISR          = 6,           /* Not allowed in ISR context         */
  BSP_W5500_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}bsp_w5500_status_t;

typedef enum
{
    BCC_CHECK_ERROR = 0 ,
    BCC_CHECK_OK 
}BCC_CHECK_STATE;

typedef enum
{
	DHCP_OFF	= 0,
	DHCP_ON		= 1,
}state_of_dhcp;

extern volatile uint8_t w5500_sock_int_status[W5500_SOCKET_NUM];
extern volatile uint8_t w5500_irq_flag ;

void bsp_read_ip_config(void );
void bsp_save_new_ip(void);
bsp_w5500_status_t bsp_net_w5500_init(void);
void W5500_DHCP_Config(void);
void W5500_ISR(void);
void W5500_Config(void);
void w5500_send_func(uint8_t sn,uint8_t *buf, uint16_t len);
void bsp_w5500_init(void);
//******************************** Declaring ********************************//


#endif


