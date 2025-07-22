/*
 * dpot_AD5292.h
 *
 * https://www.analog.com/jp/products/ad5292.html
 *
 *  Created on: Jul 16, 2025
 *      Author: ssshu
 */

#ifndef DPOT_AD5292_H
#define DPOT_AD5292_H

#include "stm32f4xx_hal.h"


#define AD5292_MIN   0x0400  /* データシート既定下限 */
#define AD5292_MAX   0x07FF  /* 12bit 上限 (2047)   */


#define AD5292_FULL_SCALE_OHMS  20000UL   /* 追記：フルスケール抵抗値 (20 kΩ品なら 20000Ω) */

void AD5292_Init(SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 GPIO_TypeDef *rdy_port, uint16_t rdy_pin);

HAL_StatusTypeDef AD5292_Set(uint32_t ohms);
HAL_StatusTypeDef AD5292_Increment(int step);   /* ±step 増減 */
uint16_t          AD5292_Get(void);

#endif /* DPOT_AD5292_H */
