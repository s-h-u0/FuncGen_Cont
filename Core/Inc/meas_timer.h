/*
 * meas_timer.h
 * 周期は MEAS_TIMER_PERIOD_MS で指定（既定 1000ms）
 */
#ifndef MEAS_TIMER_H
#define MEAS_TIMER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// === 周期／ため込み上限（必要なら上書き） ===
#ifndef MEAS_TIMER_PERIOD_MS
#define MEAS_TIMER_PERIOD_MS 200U   // 例: 200 にすると 200ms
#endif
#ifndef MEAS_TIMER_MAX_PENDING
#define MEAS_TIMER_MAX_PENDING 1U    // 貯めない（実質ワンショット挙動）
#endif

// TIMハンドルを登録（例: &htim5）; Init時にARRを上書きします
void MeasTimer_Init(TIM_HandleTypeDef *htim);

// 割り込み開始／停止（HAL_TIM_Base_Start_IT/Stop_IT を内部で呼ぶ）
void MeasTimer_Start(void);
void MeasTimer_Stop(void);

// HALのPeriodElapsedコールバックから呼ぶ
void MeasTimer_OnPeriodElapsed(TIM_HandleTypeDef *htim);

// ペンディングを1回分だけ消費（立っていればtrue）
bool MeasTimer_Consume(void);

// デバッグ用
uint8_t  MeasTimer_Pending(void);
uint32_t MeasTimer_IrqCount(void);

#ifdef __cplusplus
}
#endif

#endif /* MEAS_TIMER_H */
