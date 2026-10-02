#ifndef __STM32_TM1637_H
#define __STM32_TM1637_H


typedef enum {
    BRIGHTNESS_0 = 0,
    BRIGHTNESS_1,
    BRIGHTNESS_2,
    BRIGHTNESS_3,
    BRIGHTNESS_4,
    BRIGHTNESS_5,
    BRIGHTNESS_6,
    BRIGHTNESS_7,
    BRIGHTNESS_8
} tm1637_brightness_t;



void bsp_display_hard_init(uint8_t brightness);
void tm1637Init(void);
void tm1637DisplayDecimal(int v, int displaySeparator);
void tm1637SetBrightness(char brightness);
void Tube_Dis(uint8_t *Dis);
void Tube_DisNum(uint8_t *Dis);
void tm1637_dis(uint8_t *Dis);
void tm1637_power_led(void);
void bsp_system_open_display(uint32_t delay_time );
void Led_Dis(void);
#endif
