#ifndef __APP_W5500_H__
#define __APP_W5500_H__

#include "main.h"
#include "stdio.h"
#include "stdint.h"

#define OPT_NAME                "OPT"
#define CST_NAME 	            "Light_Machine" 

#define UCHAR                   unsigned char
#define USHORT                  unsigned short
#define ULONG                   unsigned long

#define NET_MODULE_CMD_SET      0X01    //配置网络中的CH9121
#define NET_MODULE_CMD_GET      0X02    //获取某个CH9121的配置
#define NET_MODULE_CMD_RESET    0X03    //恢复出厂设置
#define NET_MODULE_CMD_SEARCH   0X04    //搜索网络中的CH9121

#define NET_MODULE_CMD_SET_BAUD 0X09        //配置网络中的模块

///CH9121网络模块命令定义
#define NET_MODULE_ACK_SET      0X81    //回应配置命令码
#define NET_MODULE_ACK_GET      0X82    //回应获取命令码
#define NET_MODULE_ACK_RESET    0X83    //获取某个CH9121的配置
#define NET_MODULE_ACK_SEARCH   0X84    //回应所搜命令码
#define NET_MODULE_ACK_SET_BAUD 0X89    //配置网络中的模块

#define CH9121_FLAG 			      "CH9121_CFG_FLAG"
#define CH9121_FLAG_LEN         16

typedef struct _DEVICEHW_CONFIG
{
  UCHAR  bDevType;                /* 设备类型，只读 */
  UCHAR  bAuxDevType;             /* 设备子类型, 只读*/
  UCHAR  bIndex;                  /* 设备序号, 只读*/
  UCHAR  bDevHardwareVer;         /* 设备硬件版本号,只读 */  
  UCHAR  bDevSoftwareVer;         /* 设备软件版本号,只读 */  
  UCHAR  szModulename[21];        /* 用户名同CH9121名*/
  UCHAR  bDevMAC[6];              /* CH9121网络MAC地址 */  
  UCHAR  bDevIP[4];               /* CH9121IP地址*/
  UCHAR  bDevGWIP[4];             /* CH9121网关IP */
  UCHAR  bDevIPMask[4];           /* CH9121子网掩码 */
  UCHAR  bDhcpEnable;             /* DHCP 使能，是否启用DHCP,1:启用，0：不启用*/
  
  //USHORT breserved1;              /* 预留暂未启用 */
  //UCHAR  breserved2[8];           /* 预留暂未启用*/
  //UCHAR  breserved3;              /* 预留暂未启用*/
  //UCHAR  breserved4[8];           /* 预留暂未启用*/
  //UCHAR  breserved5;              /* 预留暂未启用*/
  //UCHAR  bComcfgEn;               /* 串口协商配置标志 1：启用 0：禁用*/
  //UCHAR  breserved6[8];           /* 预留暂未启用*/
  
}DeviceHWConfigS,*pDeviceHWConfigS;

extern DeviceHWConfigS gDeviceHWConfigS;

void ch9121_config_init(void );
void app_upd_handle(uint8_t s_sn);
void app_udp_dhcp_handle(uint8_t s_sn);
void app_tcp_handle(uint8_t sn);	

#endif // __APP_W5500_H__
