/*
 * dipsw_221AMA16R.h
 *
 * 221-AMA16R 4-bit DIPSW Driver
 * https://www.digikey.jp/ja/products/detail/cts-electrocomponents/221AMA16R/5227981
 *
 * Created on: Jul 16, 2025
 * Author     : ssshu
 */
#ifndef DIPSW_221AMA16R_H
#define DIPSW_221AMA16R_H

#include "stm32f4xx_hal.h"

/*------------------------------------------------------------
 * 端子  ↔ MCU ピン (必要ならここだけ書き換え)
 *-----------------------------------------------------------*/
#ifndef DIP_BIT1_GPIO_PORT   /* 1 (LSB) → PD7 */
#define DIP_BIT1_GPIO_PORT  GPIOD
#define DIP_BIT1_GPIO_PIN   GPIO_PIN_7
#endif
#ifndef DIP_BIT2_GPIO_PORT   /* 2        → PD6 */
#define DIP_BIT2_GPIO_PORT  GPIOD
#define DIP_BIT2_GPIO_PIN   GPIO_PIN_6
#endif
#ifndef DIP_BIT4_GPIO_PORT   /* 4        → PD5 */
#define DIP_BIT4_GPIO_PORT  GPIOD
#define DIP_BIT4_GPIO_PIN   GPIO_PIN_5
#endif
#ifndef DIP_BIT8_GPIO_PORT   /* 8 (MSB) → PD4 */
#define DIP_BIT8_GPIO_PORT  GPIOD
#define DIP_BIT8_GPIO_PIN   GPIO_PIN_4
#endif
/*-----------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* CubeMX でピン初期化済みの場合は不要
void DIP221_Init(void);
*/

/* DIP 値読み取り (0-15)  */
uint8_t DIP221_Read(void);

#ifdef __cplusplus
}
#endif
#endif /* DIPSW_221AMA16R_H */
