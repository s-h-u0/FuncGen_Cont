/*
 * dpot_AD5292.c
 *
 * https://www.analog.com/jp/products/ad5292.html
 *
 *  Created on: Jul 16, 2025
 *      Author: ssshu
 */

#include <dpot_AD5292.h>

#define CMD_WRITE  0x1802
#define CMD_WRITE_CLR 0x1800

static SPI_HandleTypeDef *ad5292_spi;
static GPIO_TypeDef *ad5292_cs_port;
static uint16_t      ad5292_cs_pin;
static GPIO_TypeDef *ad5292_rdy_port;
static uint16_t      ad5292_rdy_pin;
static uint16_t      ad5292_wiper = AD5292_MIN;

static inline void CS_Low (void){ HAL_GPIO_WritePin(ad5292_cs_port, ad5292_cs_pin, GPIO_PIN_RESET); }
static inline void CS_High(void){ HAL_GPIO_WritePin(ad5292_cs_port, ad5292_cs_pin, GPIO_PIN_SET   ); }


static HAL_StatusTypeDef SPI_TxWord(uint16_t data)
{
    return HAL_SPI_Transmit(ad5292_spi, (uint8_t*)&data, 1, HAL_MAX_DELAY);
}


static void waitReady(void)
{
    /* RDY ピンが接続されていない場合はスキップ可能 */
    if (ad5292_rdy_port==NULL) return;
    while (HAL_GPIO_ReadPin(ad5292_rdy_port, ad5292_rdy_pin)==GPIO_PIN_RESET) {
        HAL_Delay(1);
    }
}


void AD5292_Init(SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef 	   *cs_port,  uint16_t cs_pin,
                 GPIO_TypeDef      *rdy_port, uint16_t rdy_pin)
{
    ad5292_spi      = hspi;
    ad5292_cs_port  = cs_port;
    ad5292_cs_pin   = cs_pin;
    ad5292_rdy_port = rdy_port;
    ad5292_rdy_pin  = rdy_pin;

    CS_High();
    HAL_Delay(1);
    AD5292_Set(ad5292_wiper);  /* 既定値を書き込み */
}


/* 抵抗値 → ワイパ・コード (0x0400–0x07FF) へ変換 */

/*
static uint16_t OhmsToCode(uint32_t ohms)
{
    if (ohms > AD5292_FULL_SCALE_OHMS)      ohms = AD5292_FULL_SCALE_OHMS;
    /* 0Ω でも 0x0400 が下限になる点に注意
    uint32_t steps = (ohms * 1023UL + AD5292_FULL_SCALE_OHMS / 2) / AD5292_FULL_SCALE_OHMS; // 四捨五入
    return (uint16_t)(AD5292_MIN + steps);      // 0x0400 + 0～1023
}
*/


HAL_StatusTypeDef AD5292_Set(uint32_t ad5292_wiper )
{

              //キャッシュ更新

    /* --- 4 フレーム送信　--- */
    HAL_StatusTypeDef st;

    CS_Low(); st = SPI_TxWord(CMD_WRITE);  CS_High(); if (st) return st;
    HAL_Delay(1);
    CS_Low(); st = SPI_TxWord(ad5292_wiper);  CS_High(); if (st) return st; // 同じwiper_valを2回送信
    HAL_Delay(1);
    CS_Low(); st = SPI_TxWord(ad5292_wiper);  CS_High(); if (st) return st;
    HAL_Delay(1);
    CS_Low(); st = SPI_TxWord(CMD_WRITE_CLR); CS_High(); if (st) return st;

    waitReady();
    return HAL_OK;
}


HAL_StatusTypeDef AD5292_Increment(int step)
{
    int32_t new_val = (int32_t)ad5292_wiper + step;
    if (new_val < AD5292_MIN) new_val = AD5292_MIN;
    if (new_val > AD5292_MAX) new_val = AD5292_MAX;
    return AD5292_Set((uint16_t)new_val);
}


uint16_t AD5292_Get(void)
{
    return ad5292_wiper;
}
