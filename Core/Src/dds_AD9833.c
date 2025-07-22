/*
 * dds_AD9833.c
 *
 * https://www.analog.com/jp/products/ad9833.html
 *
 *  Created on: Jul 16, 2025
 *      Author: ssshu
 */

#include <dds_AD9833.h>

static SPI_HandleTypeDef *ad9833_spi = NULL;

void AD9833_Init(SPI_HandleTypeDef *hspi)
{
    ad9833_spi = hspi;

}

void AD9833_Set(uint32_t 		 freq_hz,
                AD9833_WaveType  wave,
                uint16_t 		 phase_deg)
{
    if (ad9833_spi == NULL) return;

    /* --- 引数より周波数・位相のコマンドを作成 --- */
    uint64_t fword = ((uint64_t)freq_hz << 28) / AD9833_REF_CLK;
    uint16_t LSB = (uint16_t)(fword & 0x3FFF) | 0x4000;
    uint16_t MSB = (uint16_t)((fword >> 14) & 0x3FFF) | 0x4000;

    uint32_t pword = (uint32_t)phase_deg * 4096UL / 360UL;
    uint16_t PH   = ((uint16_t)pword & 0x0FFF) | 0xC000;

    /* --- コマンドを格納 --- */
    uint16_t seq[5] = {
        0x2100,
        LSB,
        MSB,
        PH,
        (uint16_t)wave
    };



    /* ----------- 書きこみ開始 ----------- */

    HAL_SPI_Transmit(ad9833_spi, (uint8_t*)seq, 5, HAL_MAX_DELAY);

    /* ----------- 書きこみ終了 ----------- */

}
