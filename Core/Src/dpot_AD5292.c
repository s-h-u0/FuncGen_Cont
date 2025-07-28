#include "dpot_AD5292.h"

// 内部状態
static SPI_HandleTypeDef *ad5292_spi   = NULL;
static GPIO_TypeDef      *ad5292_cs_port = NULL;
static uint16_t           ad5292_cs_pin  = 0;
static GPIO_TypeDef      *ad5292_rdy_port= NULL;
static uint16_t           ad5292_rdy_pin = 0;

// 1 フレーム送信＋RDY 待ち
static void sendFrame(uint16_t word)
{
    HAL_GPIO_WritePin(ad5292_cs_port, ad5292_cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(ad5292_spi, (uint8_t*)&word, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ad5292_cs_port, ad5292_cs_pin, GPIO_PIN_SET);
    while (HAL_GPIO_ReadPin(ad5292_rdy_port, ad5292_rdy_pin) == GPIO_PIN_RESET) {
        HAL_Delay(1);
    }
}

void AD5292_Init(SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 GPIO_TypeDef *rdy_port, uint16_t rdy_pin)
{
    ad5292_spi     = hspi;
    ad5292_cs_port = cs_port;
    ad5292_cs_pin  = cs_pin;
    ad5292_rdy_port= rdy_port;
    ad5292_rdy_pin = rdy_pin;
    // CS 非選択 (High)
    HAL_GPIO_WritePin(ad5292_cs_port, ad5292_cs_pin, GPIO_PIN_SET);
}

void AD5292_Set(uint16_t wiper_val)
{
    const uint16_t CMD_WRITE     = 0x1802;
    const uint16_t CMD_WRITE_CLR = 0x1800;
    sendFrame(CMD_WRITE);
    sendFrame(wiper_val & 0x0FFF);
    sendFrame(wiper_val & 0x0FFF);
    sendFrame(CMD_WRITE_CLR);
}

static uint16_t voltageToWiper(uint32_t volt)
{
    uint32_t range = AD5292_MAX - AD5292_MIN;
    if (volt > VOLTAGE_MAX_V) volt = VOLTAGE_MAX_V;
    // 四捨五入込みのステップ計算
    uint32_t stepped = (volt * range + VOLTAGE_MAX_V/2U) / VOLTAGE_MAX_V;
    return (uint16_t)(AD5292_MIN + stepped);
}

void AD5292_SetVoltage(uint32_t volt)
{
    AD5292_Set(voltageToWiper(volt));
}
