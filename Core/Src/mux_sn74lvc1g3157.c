/*
 * mux_sn74lvc1g3157.c
 *
 * https://www.ti.com/product/ja-jp/SN74LVC1G3157
 *
 *  Created on: Jul 16, 2025
 *      Author: ssshu
 */


#include <mux_sn74lvc1g3157.h>

static SwitchChannel current_ch = SWITCH_CH_B1 ;

void CLK_MuxInit(void)
{
    HAL_GPIO_WritePin(SWITCH_S_GPIO_PORT, SWITCH_S_GPIO_PIN, GPIO_PIN_RESET);
    current_ch = SWITCH_CH_B1 ;
}

void CLK_MuxSet(SwitchChannel ch)
{
    current_ch = ch;
    HAL_GPIO_WritePin(SWITCH_S_GPIO_PORT,
                      SWITCH_S_GPIO_PIN,
                      (ch == SWITCH_CH_B2 ) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

SwitchChannel CLK_MuxGet(void)
{
	return HAL_GPIO_ReadPin(SWITCH_S_GPIO_PORT, SWITCH_S_GPIO_PIN) ? SWITCH_CH_B2  : SWITCH_CH_B1 ;

}
