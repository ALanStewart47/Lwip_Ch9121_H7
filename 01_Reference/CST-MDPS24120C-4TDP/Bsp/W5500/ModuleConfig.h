
// 定义了与网络CH9121通信的基本数据结构，和配置结构，参数等

#ifndef __MODULECONFIG_H__
#define __MODULECONFIG_H__

//变量类型定义
#define UCHAR  unsigned char
#define USHORT unsigned short
#define ULONG  unsigned long

#define NET_MODULE_DATA_LENGTH    255   //与CH9121通信时数据区的最大长度

#define MODULE_CFG_LEN_NEW   ( 74 + 65*2 )

//通信命令码
#define NET_MODULE_CMD_SET      0X01    //配置网络中的CH9121
#define NET_MODULE_CMD_GET      0X02    //获取某个CH9121的配置
#define NET_MODULE_CMD_RESET    0X03    //获取某个CH9121的配置
#define NET_MODULE_CMD_SEARCH   0X04    //搜索网络中的CH9121

#define NET_MODULE_CMD_SET_BAUD			0X09		//配置网络中的模块

//应答命令码
#define NET_MODULE_ACK_SET      0X81    //回应配置命令码
#define NET_MODULE_ACK_GET      0X82    //回应获取命令码
#define NET_MODULE_ACK_RESET    0X83    //获取某个CH9121的配置
#define NET_MODULE_ACK_SEARCH   0X84    //回应所搜命令码

#define NET_MODULE_ACK_SET_BAUD			0X89		//配置网络中的模块

//校验和错误时的应答命令码
#define NET_MODULE_NAK_SEARCH    0xC0    //搜索校验错
#define NET_MODULE_NAK_SET       0XC1    //配置校验错
#define NET_MODULE_NAK_GET       0XC2    //获取校验错

#define CH9121_CFG_FLAG        "CH9121_CFG_FLAG"    //用来标识通信
uint8_t CH9121_NC_84[2]={0,0x84}; 
uint8_t CH9121_NC_82[2]={0,0x82}; 
uint8_t CH9121_NC_81[2]={0,0x81}; 
uint8_t CH9121_len_12=0x0c;
uint8_t CH9121_len_204=0xcc;
uint8_t CH9121_CFG_FLAG_1[15]={'C','H','9','1','2','1','_','C','F','G','_','F','L','A','G'};
uint8_t CH9121_NC_end[3]={0x20,0,0x23}; 
uint8_t CH9121_2121[5]={0x21,0x21,0x01,0x02,0x03}; 




//网络通信结构体
typedef struct NET_COMM {
  unsigned char flag[16];                     //通信标识，因为都是用广播方式进行通信的，所以这里加一个固定值
  unsigned char cmd;                          //命令头
  unsigned char id[6];                        //CH9121MAC地址
  unsigned char pcid[6];                      //PC的MAC地址
  unsigned char len;                          //数据区长度
  unsigned char data[NET_MODULE_DATA_LENGTH]; //数据区缓冲区
}net_comm,*pnet_comm;

//CH9121标识
#define  NET_MODULE_TYPE_TCP_S    0X00         //CH9121作为TCP SERVER
#define  NET_MODULE_TYPE_TCP_C    0X01         //CH9121作为TCP CLIENT
#define  NET_MODULE_TYPE_UDP_S    0X02         //CH9121作为UDP SERVER
#define NET_MODULE_TYPE_UDP_C     0X03         //CH9121作为UDP CLIENT

//校验位标识
#define NET_MODULE_VERIFY_NULL    0X04         //无校验
#define NET_MODULE_VERIFY_ODD     0X00         //奇校验
#define NET_MODULE_VERIFY_EVEN    0X01         //偶校验
#define NET_MODULE_VERIFY_MARK    0X02         //mark校验
#define NET_MODULE_VERIFY_SPACE   0X03         //space校验


/* 设备硬件信息结构体定义 */
#define CH9121_DEV_TYPE             0x30
#define CH9121_DEV_AUX_TYPE         0x01
#define CH9121_DEV_HARDWARE_VER     0x02
#define CH9121_DEV_SOFTWARE_VER     0x02   

/* 子设备配置信息结构体定义 */

#define CH9121_DevType         0x21
#define CH9121_AuxDevType      0x21
#define CH9121_DevbIndex       0x01
#define CH9121_DevHardwareVer  0x02
#define CH9121_SoftwareVer     0x02
#define CH9121_VER    (((CH9121_DevHardwareVer<<4) & 0xF0) | (CH9121_SoftwareVer & 0x0f))



__packed typedef struct _DEVICEHW_CONFIG
{
  UCHAR  bDevType;                /* 设备类型，只读 */
  UCHAR  bAuxDevType;             /* 设备子类型, 只读*/
  UCHAR  bIndex;                  /* 设备序号, 只读*/
  UCHAR  bDevHardwareVer;         /* 设备硬件版本号,只读 */  
  UCHAR  bDevSoftwareVer;         /* 设备软件版本号,只读 */  
  UCHAR  szModulename[21];        /* 用户名同CH9121名*/
  UCHAR  bDevMAC[ 6 ];            /* CH9121网络MAC地址 */  
  UCHAR  bDevIP[ 4 ];             /* CH9121IP地址*/
  UCHAR  bDevGWIP[ 4 ];           /* CH9121网关IP */
  UCHAR  bDevIPMask[ 4 ];         /* CH9121子网掩码 */
  UCHAR  bDhcpEnable;             /* DHCP 使能，是否启用DHCP,1:启用，0：不启用*/
  USHORT breserved1;              /* 预留暂未启用 */
  UCHAR  breserved2[8];           /* 预留暂未启用*/
  UCHAR  breserved3;              /* 预留暂未启用*/
  UCHAR  breserved4[8];           /* 预留暂未启用*/
  UCHAR  breserved5;              /* 预留暂未启用*/
  UCHAR  bComcfgEn;               /* 串口协商配置标志 1：启用 0：禁用*/
  UCHAR  breserved6[8];           /* 预留暂未启用*/
}DeviceHWConfigS,*pDeviceHWConfigS;

__packed typedef struct _DEVICEPORT_CONFIG
{
  UCHAR  bIndex;                   /* 子设备序号，只读  */
  UCHAR  bPortEn;                  /* 端口启用标志 1：启用 ；0：不启用 */
  UCHAR  bNetMode;                 /* 网络工作模式: 0: TCP SERVER;1: TCP CLENT; 2: UDP SERVER 3：UDP CLIENT; */
  UCHAR  bRandSportFlag;           /* TCP 客户端模式下随机本地端口号，1：随机 0: 不随机*/
  USHORT wNetPort;                 /* 本地端口号 */  
  UCHAR  bDesIP[ 4 ];              /* 目的IP地址 */
  USHORT wDesPort;                 /* 目的端口号 */
  ULONG  dBaudRate;                /* 串口波特率: 300---921600bps */
  UCHAR  bDataSize;                /* 串口数据位: 5---8位 */  
  UCHAR  bStopBits;                /* 串口停止位: 0表示1个停止位; 1表示1.5个停止位; 2表示2个停止位 */  
  UCHAR  bParity;                  /* 串口校验位: 4表示无校验，0表示奇校验; 1表示偶校验; 2表示标志位(MARK,置1); 3表示空白位(SPACE,清0);  */    
  UCHAR  bPHYChangeHandle;         /* PHY断开，Socket动作，1：关闭Socket 0：不动作*/
  ULONG  dRxPktlength;             /* 串口RX数据打包长度，最大1024 */
  ULONG  dRxPktTimeout;            /* 串口RX数据打包转发的最大等待时间,单位为: 10ms,0则表示关闭超时功能 */
  UCHAR  bResv;                    /* 预留未启用*/  
  UCHAR  bResetCtrl;               /* 串口复位操作: 0表示不清空串口数据缓冲区; 1表示连接时清空串口数据缓冲区 */  
  UCHAR  bDNSFlag;                 /* 域名功能启用标志，1：启用 0：不启用*/
  UCHAR  szDomainname[20];         /* TCP客户端模式下，目的地址，域名*/
  UCHAR  breserved[14];            /* 保留*/
}DevicePortConfigS,*pDevicePortConfigS;


__packed typedef struct _NET_DEVICE_CONFIG
{   
  DeviceHWConfigS     HWCfg;        /*网络参数结构体*/
  DevicePortConfigS   PortCfg[2];   /*透传通道参数*/
}NetDeviceConfigS,*pNetDeviceConfigS;



#endif
