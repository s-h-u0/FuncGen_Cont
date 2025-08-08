/*
 * meas_timer.h
 *
 *  Created on: Aug 8, 2025
 *      Author: ssshu
 */

// meas_timer.h
#ifndef MEAS_TIMER_H
#define MEAS_TIMER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// TIMハンドルを登録（例: &htim5）
void MeasTimer_Init(TIM_HandleTypeDef *htim);

// 割り込み開始／停止（HAL_TIM_Base_Start_IT/Stop_IT を内部で呼ぶ）
void MeasTimer_Start(void);
void MeasTimer_Stop(void);

// HALのPeriodElapsedコールバックから呼び出す転送口
void MeasTimer_OnPeriodElapsed(TIM_HandleTypeDef *htim);

// 1秒要求フラグを1回ぶん消費（あればtrueを返す）
bool MeasTimer_Consume(void);

// ため込まれている要求数を参照（デバッグ用）
uint8_t MeasTimer_Pending(void);

#ifdef __cplusplus
}
#endif

#endif /* MEAS_TIMER_H */
