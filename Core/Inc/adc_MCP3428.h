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

/** 1LSB あたりの電圧[mV]: 2.048V/32768*1000 */
#define RESOLUTION_FACTOR_MV  (2.048f / 32768.0f * 1000.0f)
/** 分圧比 */
#define ATTENUATION           (11.0f)
/** ゲイン補正 */
#define GAIN_CALIB            (0.998f)

typedef enum {
    MCP3428_GAIN_1 = 1,
    MCP3428_GAIN_2 = 2,
    MCP3428_GAIN_4 = 4,
    MCP3428_GAIN_8 = 8,
} MCP3428_Gain;

typedef enum {
    MCP3428_SPS_12BIT = 12,
    MCP3428_SPS_14BIT = 14,
    MCP3428_SPS_16BIT = 16,
} MCP3428_Resolution;

bool MCP3428_adc_init(I2C_HandleTypeDef* hi2c, uint8_t addr7bit);
bool MCP3428_start_conversion(uint8_t channel, MCP3428_Resolution res, MCP3428_Gain gain);
bool MCP3428_check_conversion_complete(void);
int16_t MCP3428_adc_read_raw(void);
int16_t MCP3428_adc_read_millivolt(void);

#endif  // ADC_MCP3428_H

#ifdef __cplusplus
}
#endif
