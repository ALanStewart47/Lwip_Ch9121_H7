#include "main.h"

#include "stm32_tm1637.h"
#include "app_light.h"
#include "app_key_display.h"

void _tm1637Start(void);
void _tm1637Stop(void);
void _tm1637ReadResult(void);
void _tm1637WriteByte(unsigned char b);
void _tm1637DelayUsec(unsigned int i);
void _tm1637ClkHigh(void);
void _tm1637ClkLow(void);
void _tm1637DioHigh(void);
void _tm1637DioLow(void);

// Configuration.

#define CLK_PORT 				TM1637_DIO_GPIO_Port
#define DIO_PORT 				TM1637_DIO_GPIO_Port
#define CLK_PIN 				TM1637_CLK_Pin
#define DIO_PIN 				TM1637_DIO_Pin
#define CLK_PORT_CLK_ENABLE 	__HAL_RCC_GPIOA_CLK_ENABLE
#define DIO_PORT_CLK_ENABLE 	__HAL_RCC_GPIOA_CLK_ENABLE


const char segmentMap[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, // 0-7
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, // 8-9, A-F
    0x76, // 'H'
    0x00,
    0x3e  //U
};

void bsp_display_hard_init(uint8_t brightness)
{
    tm1637Init();
    tm1637SetBrightness(brightness);
}

void tm1637Init(void)
{
    CLK_PORT_CLK_ENABLE();
    DIO_PORT_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_OUTPUT_PP; // OD = open drain
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Pin = CLK_PIN;
    HAL_GPIO_Init(CLK_PORT, &g);
    g.Pin = DIO_PIN;
    HAL_GPIO_Init(DIO_PORT, &g);
}

void tm1637DisplayDecimal(int v, int displaySeparator)
{
    unsigned char digitArr[5];
    /*for (int i = 0; i < 5; i++) 
	{
        digitArr[i] = segmentMap[v % 10];
        
        if (i == 2 && displaySeparator) 
		{
            digitArr[i] |= 1 << 7;
        }
        v /= 10;
    }*/
    int temp = v;
    for (int i = 0; i < 5; i++) 
    {
        digitArr[4-i] = segmentMap[temp % 10];  // 反向存储，使得从左到右显示
        if (i == 2 && displaySeparator) 
        {
            //digitArr[4-i] |= 1 << 7;  // 在指定位置添加小数点
        }
        temp /= 10;
    }

    _tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();

    _tm1637Start();
    _tm1637WriteByte(0xc0);
    _tm1637ReadResult();

    for (int i = 0; i < 6; i++) {
        _tm1637WriteByte(digitArr[i]);
        _tm1637ReadResult();
    }

    _tm1637Stop();
}


void tm1637_dis(uint8_t *Dis)
{
	_tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();
    _tm1637Start();
    _tm1637WriteByte(0xc0);
    _tm1637ReadResult();

    for (int i = 0; i < 6; i++) {
        _tm1637WriteByte(Dis[i]);
        _tm1637ReadResult();
    }
    _tm1637Stop();
}

void Tube_Dis(uint8_t *Dis)
{
	_tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();
    _tm1637Start();
    _tm1637WriteByte(0xc0);
    _tm1637ReadResult();

    for (int i = 0; i < 5; i++) {
        _tm1637WriteByte(segmentMap[Dis[i]]);
        _tm1637ReadResult();
    }
    _tm1637Stop();
}
void Tube_DisNum(uint8_t *Dis)
{
	_tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();
    _tm1637Start();
    _tm1637WriteByte(0xc0);
    _tm1637ReadResult();

    for (int i = 0; i < 5; i++) {
        _tm1637WriteByte(segmentMap[Dis[i]]);
        _tm1637ReadResult();
    }
    _tm1637Stop();
}

void Led_Dis(void)
{
    //uint8_t work_mode = 0;
    uint8_t page = 0;
    //static uint8_t old_work_mode = 0;
    uint8_t Dis = 0x01;

    _tm1637Start();
    _tm1637WriteByte(0x40);
    _tm1637ReadResult();
    _tm1637Stop();
    _tm1637Start();
    _tm1637WriteByte(0xc5);
    _tm1637ReadResult();

    if(get_light_alarm() != STATUS_NORMAL)
         Dis |= 0x08; 

    page = bsp_get_main_page();
    if(page == PAGE_BRIGHTNESS_NORMAL)
    {
        Dis |= 0x02; //亮度页面
    }
    else if(page == PAGE_STROBE_WIDTH) 
    {
        Dis |= 0x04; //频闪宽度页面
    }
    /*else if(page == PAGE_WORK_MODE)
    {
        Dis |= 0x06; //工作模式页面
    }*/
    /*
    work_mode = get_light_mode();
    if(work_mode == WORK_MODE_NORMAL)
        Dis |= 0x02; 
    else if(work_mode == WORK_MODE_STROBE)
        Dis |= 0x04; 
    else if(work_mode == WORK_MODE_PROG)
        Dis |= 0x06;*/
    
    _tm1637WriteByte(Dis);
    _tm1637ReadResult();
    _tm1637Stop();
}


void bsp_system_open_display(uint32_t delay_time )
{
    
    uint8_t digitArr[6] = {0}; // Display "no" with LED1 on
    uint32_t s_time = 0;

    Tube_DisNum(digitArr);
    s_time = delay_time/4;

    for(uint8_t i = 0; i < 6; i++)
    {
        digitArr[i] = 0x40; // 14 = '-'
        tm1637_dis(digitArr);
        extern void bsp_feedDog(void);
        bsp_feedDog();
        HAL_Delay(s_time);
    }
}


// Valid brightness values: 0 - 8.
// 0 = display off.
void tm1637SetBrightness(char brightness)
{
    // Brightness command:
    // 1000 0XXX = display off
    // 1000 1BBB = display on, brightness 0-7
    // X = don't care
    // B = brightness
    _tm1637Start();
    _tm1637WriteByte(0x87 + brightness);
    _tm1637ReadResult();
    _tm1637Stop();
}

void _tm1637Start(void)
{
    _tm1637ClkHigh();
    _tm1637DioHigh();
    _tm1637DelayUsec(2);
    _tm1637DioLow();
}

void _tm1637Stop(void)
{
    _tm1637ClkLow();
    _tm1637DelayUsec(2);
    _tm1637DioLow();
    _tm1637DelayUsec(2);
    _tm1637ClkHigh();
    _tm1637DelayUsec(2);
    _tm1637DioHigh();
}

void _tm1637ReadResult(void)
{
    _tm1637ClkLow();
    _tm1637DelayUsec(5);
    // while (dio); // We're cheating here and not actually reading back the response.
    _tm1637ClkHigh();
    _tm1637DelayUsec(2);
    _tm1637ClkLow();
}

void _tm1637WriteByte(unsigned char b)
{
    for (int i = 0; i < 8; ++i) {
        _tm1637ClkLow();
        if (b & 0x01) {
            _tm1637DioHigh();
        }
        else {
            _tm1637DioLow();
        }
        _tm1637DelayUsec(3);
        b >>= 1;
        _tm1637ClkHigh();
        _tm1637DelayUsec(3);
    }
}

void _tm1637DelayUsec(unsigned int i)
{
    for (; i>0; i--) {
        for (int j = 0; j < 10; ++j) {
            __NOP();
        }
    }
}

void _tm1637ClkHigh(void)
{
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
}

void _tm1637ClkLow(void)
{
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
}

void _tm1637DioHigh(void)
{
    HAL_GPIO_WritePin(DIO_PORT, DIO_PIN, GPIO_PIN_SET);
}

void _tm1637DioLow(void)
{
    HAL_GPIO_WritePin(DIO_PORT, DIO_PIN, GPIO_PIN_RESET);
}
