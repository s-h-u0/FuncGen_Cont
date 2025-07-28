#ifndef DPOT_AD5292_H
#define DPOT_AD5292_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/// ワイパーコードの範囲 (0x400〜0x7FF)
#define AD5292_MIN      (0x400U)
#define AD5292_MAX      (0x7FFU)
/// マッピング対象の最大電圧[V]
#define VOLTAGE_MAX_V   (40U)

/**
 * @brief  ドライバ初期化 (SPI, CS, RDY をセット)
 */
void AD5292_Init(SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 GPIO_TypeDef *rdy_port, uint16_t rdy_pin);

/**
 * @brief  ワイパーコードを直接書き込み (AD5292_MIN〜AD5292_MAX)
 */
void AD5292_Set(uint16_t wiper_val);

/**
 * @brief  [0〜VOLTAGE_MAX_V] の電圧を線形マッピングしてワイパーに設定
 */
void AD5292_SetVoltage(uint32_t volt);

#ifdef __cplusplus
}
#endif

#endif // DPOT_AD5292_H
