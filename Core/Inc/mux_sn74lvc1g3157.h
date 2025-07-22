/*
 * mux_sn74lvc1g3157.h
 *
 * https://www.ti.com/product/ja-jp/SN74LVC1G3157
 *
 *  Created on: Jul 16, 2025
 *      Author: ssshu
 */


#ifndef MUX_SN74LVC1G3157
#define MUX_SN74LVC1G3157

#include "stm32f4xx_hal.h"   /* MCU family header。適宜変更 */

/* === GPIO マクロ定義 (ヘッダ上で変更可) ==================== */
#ifndef SWITCH_S_GPIO_PORT
#define SWITCH_S_GPIO_PORT   GPIOB      /* S 端子を接続するポート */
#endif
#ifndef SWITCH_S_GPIO_PIN
#define SWITCH_S_GPIO_PIN    GPIO_PIN_6 /* S 端子を接続するピン   */
#endif

/* === 切替チャネル列挙型 ==================================== */
typedef enum {
	SWITCH_CH_B1 = 0,   /* S=L → B1 ↔ A */
	SWITCH_CH_B2 = 1    /* S=H → B2 ↔ A */
} SwitchChannel;

#ifdef __cplusplus
extern "C" {
#endif

/* === API ==================================================== */
void  CLK_MuxInit  (void);                 /* GPIO 初期化        */
void  CLK_MuxSet   (SwitchChannel ch);     /* チャネル切替       */
SwitchChannel CLK_MuxGet(void);            /* 現在チャネル取得   */

#ifdef __cplusplus
}
#endif

#endif /* MUX_SN74LVC1G3157 */
