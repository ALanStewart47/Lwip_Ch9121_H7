/**
  ******************************************************************************
  * 文件名程: tcp_echoserver.h
  * 作    者: 硬石嵌入式开发团队
  * 版    本: V1.0
  * 编写日期: 2022-11-20
  * 功    能: tcp_echoserver头文件
  ******************************************************************************
  * 说明：
  * 本例程配套硬石stm32开发板YS-F4STD使用。
  * 
  * 淘宝：
  * 论坛：http://www.ing10bbs.com
  * 版权归硬石嵌入式开发团队所有，请勿商用。
  ******************************************************************************
  */
#ifndef __TCP_ECHOSERVER_H__
#define __TCP_ECHOSERVER_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "main.h"
#include "stdio.h"
#include "string.h"
#include "key/bsp_key.h"
#include "app_ethernet.h"
#include "ethernetif.h"
#include "lwip/timeouts.h"
#include "key/bsp_key.h"

/* 类型定义 ------------------------------------------------------------------*/
/* TCP服务器连接状态 */
enum tcp_echoserver_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* LwIP回调函数使用结构体 */
struct tcp_echoserver_struct
{
  u8_t state;             /* 当前连接状态 */
  u8_t retries;
  struct tcp_pcb *pcb;    /* 指向当前的pcb */
  struct pbuf *p;         /* 指向当前接收或传输的pbuf */
};

/* 宏定义 --------------------------------------------------------------------*/
#define TCP_SERVER_PORT			  1234	//定义tcp server的端口
#define TCP_SERVER_RX_BUFSIZE	100	  //定义tcp server最大接收数据长度

/* 扩展变量 ------------------------------------------------------------------*/
/* 函数声明 ------------------------------------------------------------------*/
void tcp_echoserver_close(void);
void YSF4_TCP_SENDData(void);


#endif /* __TCP_ECHOSERVER */

/******************* (C) COPYRIGHT 2020-2030 硬石嵌入式开发团队 *****END OF FILE****/

