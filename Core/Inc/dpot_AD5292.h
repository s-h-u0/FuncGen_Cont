#ifndef DPOT_AD5292_H
#define DPOT_AD5292_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AD5292_MIN   0x0400  /* データシート既定下限 */
#define AD5292_MAX   0x07FF  /* 12bit 上限 (2047)   */

// フルスケール抵抗値（Ω）を20kΩに
#define AD5292_FULL_SCALE_OHMS 20000UL

// システムの最大出力電圧（V）
#define MAX_OUTPUT_VOLTAGE     40UL

void AD5292_Init(SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 GPIO_TypeDef *rdy_port, uint16_t rdy_pin);

HAL_StatusTypeDef AD5292_Set(uint32_t ohms);
HAL_StatusTypeDef AD5292_Increment(int step);   /* ±step 増減 */
uint16_t          AD5292_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* DPOT_AD5292_H */
