/*
 * dipsw_221AMA16R.c
 *
 * 221-AMA16R 4-bit DIPSW Driver
 * https://www.digikey.jp/ja/products/detail/cts-electrocomponents/221AMA16R/5227981
 *
 * Created on: Jul 16, 2025
 * Author     : ssshu
 */

#include <dipsw_221AMA16R.h>



uint8_t DIP221_Read(void)
{
    uint8_t val = 0;

    if (HAL_GPIO_ReadPin(DIP_BIT1_GPIO_PORT, DIP_BIT1_GPIO_PIN) == GPIO_PIN_RESET) val |= 0x1;
    if (HAL_GPIO_ReadPin(DIP_BIT2_GPIO_PORT, DIP_BIT2_GPIO_PIN) == GPIO_PIN_RESET) val |= 0x2;
    if (HAL_GPIO_ReadPin(DIP_BIT4_GPIO_PORT, DIP_BIT4_GPIO_PIN) == GPIO_PIN_RESET) val |= 0x4;
    if (HAL_GPIO_ReadPin(DIP_BIT8_GPIO_PORT, DIP_BIT8_GPIO_PIN) == GPIO_PIN_RESET) val |= 0x8;

    return val;
}
