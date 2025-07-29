#ifdef __cplusplus
extern "C" {
#endif


#ifndef ADC_MCP3428_H
#define ADC_MCP3428_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/** MCP3428 の 7bit I2C アドレス */
#define MCP3428_DEFAULT_ADDR  0x68

/** I2C ポートと 7bit アドレス（<<1 は実装内でやります） */
bool MCP3428_adc_init(I2C_HandleTypeDef* hi2c, uint8_t addr7bit);

/** 生データをそのまま返す（16bit 2バイト読み→符号付き結合） */
int16_t MCP3428_adc_read_raw(void);

/** mV 単位で返す （分圧・係数込み） */
int16_t MCP3428_adc_read_millivolt(void);

#endif  // ADC_MCP3428_H

#ifdef __cplusplus
}
#endif
