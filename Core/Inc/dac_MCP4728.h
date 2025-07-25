/*
 * dac_MCP4728.h
 *
 *  Created on: Jul 25, 2025
 *      Author: ssshu
 */

#ifndef DAC_MCP4728_H
#define DAC_MCP4728_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

// MCP4728 I2C アドレス (STM32 HAL 用に 8bit アドレス指定)
#define MCP4728_I2C_ADDR_DEFAULT  (0x60 << 1)

typedef enum {
    MCP4728_CHANNEL_A = 0,
    MCP4728_CHANNEL_B,
    MCP4728_CHANNEL_C,
    MCP4728_CHANNEL_D
} MCP4728_Channel;

typedef enum {
    MCP4728_VREF_VDD = 0,
    MCP4728_VREF_INTERNAL
} MCP4728_Vref;

typedef enum {
    MCP4728_GAIN_1X = 0,
    MCP4728_GAIN_2X
} MCP4728_Gain;

typedef enum {
    MCP4728_PD_NORMAL = 0,
    MCP4728_PD_1K,
    MCP4728_PD_100K,
    MCP4728_PD_500K
} MCP4728_PowerDown;

bool MCP4728_Init(I2C_HandleTypeDef *hi2c);
bool MCP4728_SetChannel(MCP4728_Channel ch, uint16_t value,
                        MCP4728_Vref vref, MCP4728_Gain gain,
                        MCP4728_PowerDown pd, bool use_udac);
bool MCP4728_FastWrite(uint16_t valA, uint16_t valB,
                       uint16_t valC, uint16_t valD);
bool MCP4728_SaveToEEPROM(void);

#endif  // DAC_MCP4728_H
