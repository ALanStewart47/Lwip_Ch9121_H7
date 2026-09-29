#include "adc.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32H7开发板
//ADC驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/8/13
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

ADC_HandleTypeDef ADC1_Handler;//ADC句柄
ADC_HandleTypeDef ADC3_Handler;//ADC句柄

//初始化ADC
//ch: ADC_channels 
//通道值 0~16取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_16
void MY_ADC1_Init(void)
{ 
    ADC1_Handler.Instance=ADC1;
    ADC1_Handler.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4; 	//4分频，ADCCLK=PER_CK/4=64/4=16MHZ
    ADC1_Handler.Init.Resolution=ADC_RESOLUTION_16B;           	//16位模式
    ADC1_Handler.Init.ScanConvMode=DISABLE;                    	//非扫描模式
    ADC1_Handler.Init.EOCSelection=ADC_EOC_SINGLE_CONV;       	//关闭EOC中断
	ADC1_Handler.Init.LowPowerAutoWait=DISABLE;					//自动低功耗关闭				
    ADC1_Handler.Init.ContinuousConvMode=DISABLE;               //关闭连续转换
    ADC1_Handler.Init.NbrOfConversion=1;                        //1个转换在规则序列中 也就是只转换规则序列1 
    ADC1_Handler.Init.DiscontinuousConvMode=DISABLE;            //禁止不连续采样模式
    ADC1_Handler.Init.NbrOfDiscConversion=0;                    //不连续采样通道数为0
    ADC1_Handler.Init.ExternalTrigConv=ADC_SOFTWARE_START;      //软件触发
    ADC1_Handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;//使用软件触发
	ADC1_Handler.Init.BoostMode=ENABLE;							//BOOT模式关闭
	ADC1_Handler.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;			//有新的数据的死后直接覆盖掉旧数据
	ADC1_Handler.Init.OversamplingMode=DISABLE;					//过采样关闭
	ADC1_Handler.Init.ConversionDataManagement=ADC_CONVERSIONDATA_DR;  //规则通道的数据仅仅保存在DR寄存器里面
    HAL_ADC_Init(&ADC1_Handler);                                 //初始化 
	
	HAL_ADCEx_Calibration_Start(&ADC1_Handler,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED); //ADC校准
}

//初始化ADC3
//ch: ADC_channels 
//通道值 0~16取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_16
void MY_ADC3_Init(void)
{ 
    ADC3_Handler.Instance=ADC3;
    ADC3_Handler.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4; 	//4分频，ADCCLK=PER_CK/4=64/4=16MHZ
    ADC3_Handler.Init.Resolution=ADC_RESOLUTION_16B;           	//16位模式
    ADC3_Handler.Init.ScanConvMode=DISABLE;                    	//非扫描模式
    ADC3_Handler.Init.EOCSelection=ADC_EOC_SINGLE_CONV;       	//关闭EOC中断
	ADC3_Handler.Init.LowPowerAutoWait=DISABLE;					//自动低功耗关闭				
    ADC3_Handler.Init.ContinuousConvMode=DISABLE;               //关闭连续转换
    ADC3_Handler.Init.NbrOfConversion=1;                        //1个转换在规则序列中 也就是只转换规则序列1 
    ADC3_Handler.Init.DiscontinuousConvMode=DISABLE;            //禁止不连续采样模式
    ADC3_Handler.Init.NbrOfDiscConversion=0;                    //不连续采样通道数为0
    ADC3_Handler.Init.ExternalTrigConv=ADC_SOFTWARE_START;      //软件触发
    ADC3_Handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;//使用软件触发
	ADC3_Handler.Init.BoostMode=ENABLE;							//BOOT模式关闭
	ADC3_Handler.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;			//有新的数据的死后直接覆盖掉旧数据
	ADC3_Handler.Init.OversamplingMode=DISABLE;					//过采样关闭
	ADC3_Handler.Init.ConversionDataManagement=ADC_CONVERSIONDATA_DR;  //规则通道的数据仅仅保存在DR寄存器里面
    HAL_ADC_Init(&ADC3_Handler);                                 //初始化 
	
	HAL_ADCEx_Calibration_Start(&ADC3_Handler,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED); //ADC校准
}

//ADC底层驱动，引脚配置，时钟使能
//此函数会被HAL_ADC_Init()调用
//hadc:ADC句柄
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_Initure;
	RCC_PeriphCLKInitTypeDef  ADCClkInitStruct;
	
    __HAL_RCC_ADC12_CLK_ENABLE();           //使能ADC1/2时钟
	__HAL_RCC_ADC3_CLK_ENABLE();			//使能ADC3时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();			//开启GPIOA时钟

	ADCClkInitStruct.PeriphClockSelection=RCC_PERIPHCLK_ADC; 
	ADCClkInitStruct.AdcClockSelection=RCC_ADCCLKSOURCE_CLKP; 
	HAL_RCCEx_PeriphCLKConfig(&ADCClkInitStruct);
	
    GPIO_Initure.Pin=GPIO_PIN_5;            //PA5
    GPIO_Initure.Mode=GPIO_MODE_ANALOG;     //模拟
    GPIO_Initure.Pull=GPIO_NOPULL;          //不带上下拉
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
}

//获得ADC值
//ch: 通道值 0~16，取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_16
//返回值:转换结果
u16 Get_Adc(ADC_HandleTypeDef ADCx,u32 ch)   
{
    ADC_ChannelConfTypeDef ADC_ChanConf;
    
    ADC_ChanConf.Channel=ch;                                //通道
    ADC_ChanConf.Rank=ADC_REGULAR_RANK_1;                  	//1个序列
    ADC_ChanConf.SamplingTime=ADC_SAMPLETIME_810CYCLES_5;   //采样时间       
	ADC_ChanConf.SingleDiff=ADC_SINGLE_ENDED;  				//单边采集          		
	ADC_ChanConf.OffsetNumber=ADC_OFFSET_NONE;             	
	ADC_ChanConf.Offset=0;   
    HAL_ADC_ConfigChannel(&ADCx,&ADC_ChanConf);             //通道配置
    HAL_ADC_Start(&ADCx);                                   //开启ADC
    HAL_ADC_PollForConversion(&ADCx,10);                    //轮询转换
	return (u16)HAL_ADC_GetValue(&ADCx);	                //返回最近一次ADC规则组的转换结果
}
//获取指定通道的转换值，取times次,然后平均 
//times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc_Average(ADC_HandleTypeDef ADCx,u32 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ADCx,ch);
		delay_ms(5);
	}
	return temp_val/times;
}

//得到温度值
//返回值:温度值(扩大了100倍,单位:℃.)
short Get_Temprate(void)
{
	u32 adcx;
	short result;
 	double temperate;
    float temp=0;
    u16 ts_cal1,ts_cal2;
    
    ts_cal1=*(vu16*)(0X1FF1E820);
    ts_cal2=*(vu16*)(0X1FF1E840);
    temp=(float)((110.0f-30.0f)/(ts_cal2-ts_cal1));
	adcx=Get_Adc_Average(ADC3_Handler,ADC_CHANNEL_TEMPSENSOR,10);//读取内部温度传感器通道,10次取平均
    temperate=(float)(temp*(adcx-ts_cal1)+30);
	result=temperate*=100;					//扩大100倍.
	return result;

}
