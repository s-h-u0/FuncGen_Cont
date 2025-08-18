#include "meas_timer.h"

static TIM_HandleTypeDef *s_htim = NULL;
static volatile uint8_t  s_req = 0;        // 要求カウンタ
static volatile uint32_t s_irq_seen = 0;   // 受けたIRQ回数
volatile uint8_t  g_meas_req = 0;          // デバッグミラー
volatile uint32_t g_meas_irq = 0;          // デバッグミラー

void MeasTimer_Init(TIM_HandleTypeDef *htim)
{
    s_htim     = htim;
    s_req      = 0;
    s_irq_seen = 0;

    // 現在のARR（CubeMX生成の "1秒" 設定：例 9999）を基準に、希望msへ比例換算
    uint32_t base_arr = __HAL_TIM_GET_AUTORELOAD(htim);      // 例: 9999
    uint32_t new_arr  = (uint32_t)((uint64_t)base_arr * MEAS_TIMER_PERIOD_MS / 1000U);
    if (new_arr < 1) new_arr = 1;

    // カウンタ/フラグをクリアしてARRを上書き
    __HAL_TIM_DISABLE(htim);
    __HAL_TIM_SET_COUNTER(htim, 0);
    __HAL_TIM_SET_AUTORELOAD(htim, new_arr);
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE(htim);
}

void MeasTimer_Start(void)
{
    if (s_htim) {
        HAL_TIM_Base_Start_IT(s_htim);
    }
}

void MeasTimer_Stop(void)
{
    if (s_htim) {
        HAL_TIM_Base_Stop_IT(s_htim);
    }
}

void MeasTimer_OnPeriodElapsed(TIM_HandleTypeDef *htim)
{
    if (htim == s_htim) {
        if (s_req < MEAS_TIMER_MAX_PENDING) s_req++;  // ★ マクロでため込み上限
        g_meas_irq++;
        s_irq_seen++;
    }
}

bool MeasTimer_Consume(void)
{
    bool ret = false;
    __disable_irq();
    if (s_req) { s_req--; ret = true; }
    __enable_irq();
    g_meas_req = s_req;     // デバッグ用ミラー
    return ret;
}

uint8_t MeasTimer_Pending(void) { return s_req; }
uint32_t MeasTimer_IrqCount(void) { return s_irq_seen; }
