/*
 * dds_AD9833.h
 *
 * https://www.analog.com/jp/products/ad9833.html
 *
 *  Created on: Jul 16, 2025
 *      Author: ssshu
 */



#ifndef DDS_AD9833_H
#define DDS_AD9833_H

#include "stm32f4xx_hal.h"


/* ----------- マスタクロック ----------- */
#define AD9833_REF_CLK (25000000UL)

/* ----------- 波形選択コマンド ----------- */
typedef enum {
    AD9833_SINE     = 0x2000,
    AD9833_TRIANGLE = 0x2002,
    AD9833_SQUARE   = 0x2028
} AD9833_WaveType;

/* ----------- 初期化 ----------- */
void AD9833_Init(SPI_HandleTypeDef *hspi);
void AD9833_Set(uint32_t freq_hz,
                AD9833_WaveType wave,
                uint16_t phase_deg);

#endif /* DDS_AD9833_H */
