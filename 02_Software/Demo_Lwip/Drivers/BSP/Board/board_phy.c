#include "board_phy.h"

#define PCF8574_ADDRESS_7BIT  0x20U
#define PHY_RESET_PIN         7U
#define I2C_DELAY_US          5U

#define I2C_SCL_PIN GPIO_PIN_4
#define I2C_SDA_PIN GPIO_PIN_5

static uint8_t pcf8574_output = 0xFFU;
volatile BoardPhyStatus g_board_phy_status = BOARD_PHY_STATUS_OK;

static void i2c_delay_us(uint32_t delay_us)
{
  uint32_t cycles_per_us = SystemCoreClock / 1000000U;
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = cycles_per_us * delay_us;

  while ((uint32_t)(DWT->CYCCNT - start) < cycles)
  {
  }
}

static void i2c_scl(uint8_t high)
{
  HAL_GPIO_WritePin(GPIOH, I2C_SCL_PIN, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void i2c_sda(uint8_t high)
{
  HAL_GPIO_WritePin(GPIOH, I2C_SDA_PIN, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void i2c_start(void)
{
  i2c_sda(1U);
  i2c_scl(1U);
  i2c_delay_us(I2C_DELAY_US);
  i2c_sda(0U);
  i2c_delay_us(I2C_DELAY_US);
  i2c_scl(0U);
}

static void i2c_stop(void)
{
  i2c_scl(0U);
  i2c_sda(0U);
  i2c_delay_us(I2C_DELAY_US);
  i2c_scl(1U);
  i2c_delay_us(I2C_DELAY_US);
  i2c_sda(1U);
  i2c_delay_us(I2C_DELAY_US);
}

static void i2c_write_byte(uint8_t value)
{
  uint32_t bit;

  i2c_scl(0U);
  for (bit = 0U; bit < 8U; ++bit)
  {
    i2c_sda((value & 0x80U) != 0U);
    i2c_delay_us(I2C_DELAY_US);
    i2c_scl(1U);
    i2c_delay_us(I2C_DELAY_US);
    i2c_scl(0U);
    value <<= 1U;
  }
  i2c_sda(1U);
}

static uint8_t i2c_wait_ack(void)
{
  uint8_t ack;

  i2c_sda(1U);
  i2c_delay_us(I2C_DELAY_US);
  i2c_scl(1U);
  i2c_delay_us(I2C_DELAY_US);
  ack = (HAL_GPIO_ReadPin(GPIOH, I2C_SDA_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
  i2c_scl(0U);
  return ack;
}

static HAL_StatusTypeDef pcf8574_write(uint8_t value)
{
  HAL_StatusTypeDef result = HAL_OK;

  i2c_start();
  i2c_write_byte((uint8_t)(PCF8574_ADDRESS_7BIT << 1U));
  if (i2c_wait_ack() == 0U)
  {
    g_board_phy_status = BOARD_PHY_STATUS_ADDR_NACK;
    result = HAL_ERROR;
  }
  else
  {
    i2c_write_byte(value);
    if (i2c_wait_ack() == 0U)
    {
      g_board_phy_status = BOARD_PHY_STATUS_DATA_NACK;
      result = HAL_ERROR;
    }
    else
    {
      pcf8574_output = value;
    }
  }

  i2c_stop();
  return result;
}

static void i2c_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  gpio.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_OD;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &gpio);
  i2c_scl(1U);
  i2c_sda(1U);

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

HAL_StatusTypeDef Board_PHY_Reset(void)
{
  HAL_StatusTypeDef result;

  g_board_phy_status = BOARD_PHY_STATUS_OK;
  pcf8574_output = 0xFFU;
  i2c_gpio_init();

  /* Put every expander output in its released state before asserting reset. */
  result = pcf8574_write(pcf8574_output);
  if (result != HAL_OK)
  {
    return result;
  }

  pcf8574_output |= (uint8_t)(1U << PHY_RESET_PIN);
  result = pcf8574_write(pcf8574_output);
  if (result != HAL_OK)
  {
    return result;
  }
  HAL_Delay(100U);

  pcf8574_output &= (uint8_t)~(1U << PHY_RESET_PIN);
  result = pcf8574_write(pcf8574_output);
  if (result != HAL_OK)
  {
    return result;
  }
  HAL_Delay(100U);

  return HAL_OK;
}
