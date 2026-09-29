#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "sdram.h"
#include "usmart.h"
#include "pcf8574.h"
#include "timer.h"
#include "adc.h"
#include "rtc.h"
#include "mpu.h"
#include "malloc.h"
#include "lwip/netif.h"
#include "lwip_comm.h"
#include "lwipopts.h"
#include "tcp_client_demo.h"
#include "tcp_server_demo.h"
#include "udp_demo.h"
#include "httpd.h"
/************************************************
 ALIENTEK 阿波罗STM32H7开发板 网络实验61
 网络通信实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//加载UI
//mode:
//bit0:0,不加载;1,加载前半部分UI
//bit1:0,不加载;1,加载后半部分UI
void lwip_test_ui(u8 mode)
{
	u32 linkstate;
	u8 buf[30]; 
	POINT_COLOR=RED;
	if(mode&1<<0)
	{
		LCD_Fill(30,30,lcddev.width,110,WHITE);	//清除显示
		LCD_ShowString(30,30,200,16,16,"Apollo STM32H7");
		LCD_ShowString(30,50,200,16,16,"Ethernet lwIP Test");
		LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
		LCD_ShowString(30,90,200,16,16,"2016/7/19"); 	
	}
	if(mode&1<<1)
	{
		LCD_Fill(30,110,lcddev.width,lcddev.height,WHITE);	//清除显示
		LCD_ShowString(30,110,200,16,16,"lwIP Init Successed");
		if(lwipdev.dhcpstatus==2)sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//打印动态IP地址
		else sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//打印静态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
        linkstate=LAN8720_GetLinkState();       //获取连接状态
        if(linkstate==LAN8720_STATUS_100MBITS_FULLDUPLEX)
            LCD_ShowString(30,150,200,16,16,"eth state:100M,FullDuplex");
        else if(linkstate==LAN8720_STATUS_100MBITS_HALFDUPLEX)
            LCD_ShowString(30,150,200,16,16,"eth state:100M,HalfDuplex");
        else if(linkstate==LAN8720_STATUS_10MBITS_FULLDUPLEX)
            LCD_ShowString(30,150,200,16,16,"eth state:10M,FullDuplex");
        else if(linkstate==LAN8720_STATUS_10MBITS_HALFDUPLEX)
            LCD_ShowString(30,150,200,16,16,"eth state:10M,HalfDuplex");
		LCD_ShowString(30,170,200,16,16,"KEY0:TCP Server Test");
		LCD_ShowString(30,190,200,16,16,"KEY1:TCP Client Test");
		LCD_ShowString(30,210,200,16,16,"KEY2:UDP Test");
	}
}

int main(void)
{
    u8 t;
	u8 key;
	
    Write_Through();                        //开启强制透写！
    MPU_Memory_Protection();                //保护相关存储区域
    Cache_Enable();                         //打开L1-Cache
	HAL_Init();				        		//初始化HAL库
	Stm32_Clock_Init(160,5,2,4);  		    //设置时钟,400Mhz 
	delay_init(400);						//延时初始化
	uart_init(115200);						//串口初始化
	usmart_dev.init(200); 		    		//初始化USMART	
	LED_Init();								//初始化LED
	KEY_Init();								//初始化按键
	SDRAM_Init();                   		//初始化SDRAM
	LCD_Init();								//初始化LCD
	PCF8574_Init();                         //初始化PCF8574
    my_mem_init(SRAMIN);		            //初始化内部内存池
	my_mem_init(SRAMEX);		            //初始化外部内存池
	my_mem_init(SRAMDTCM);		            //初始化DTCM内存池
    MY_ADC1_Init();                         //初始化ADC1
    MY_ADC3_Init();                         //初始化ADC3
    RTC_Init();                             //初始化RTC 
    TIM3_Init(1000-1,2000-1);               //定时器3初始化，定时器时钟为200M，分频系数为2000-1，
                                            //所以定时器3的频率为200M/2000=100K，自动重装载为1000-1，那么定时器周期就是10ms
   	POINT_COLOR=RED;
	LED0(1);
	lwip_test_ui(1);			    //加载前半部分UI
    LCD_ShowString(30,110,200,16,16,"lwIP Initing...");
	while(lwip_comm_init())         //lwip初始化
	{
		LCD_ShowString(30,110,200,20,16,"LWIP Init Falied! ");
		delay_ms(500);
		LCD_ShowString(30,110,200,16,16,"Retrying...       ");
        delay_ms(500);
	}
	LCD_ShowString(30,110,200,20,16,"LWIP Init Success!");
 	LCD_ShowString(30,130,200,16,16,"DHCP IP configing...");  //等待DHCP获取 
#if LWIP_DHCP   //使用DHCP
	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//等待DHCP获取成功/超时溢出
	{  
		lwip_periodic_handle();	//LWIP内核需要定时处理的函数
	}
#endif
    lwip_test_ui(2);		//加载后半部分UI 
	httpd_init();	        //HTTP初始化(默认开启websever)
	while(1)
	{
		key=KEY_Scan(0);
		switch(key)
		{
			case KEY0_PRES://TCP Server模式
				tcp_server_test();
 				lwip_test_ui(3);//重新加载UI  
				break;
			case KEY1_PRES://TCP Client模式
				tcp_client_test();
				lwip_test_ui(3);//重新加载UI
				break; 
			case KEY2_PRES://UDP模式
				udp_demo_test();
				lwip_test_ui(3);//重新加载UI
				break; 
		} 
		lwip_periodic_handle();
		delay_ms(2);
		t++;
		if(t==100)LCD_ShowString(30,230,200,16,16,"Please choose a mode!");
		if(t==200)
		{ 
			t=0;
			LCD_Fill(30,230,230,230+16,WHITE);//清除显示
			LED0_Toggle;
		} 
	} 	
}

