#include "meas_timer.h"

static TIM_HandleTypeDef *s_htim = NULL;
static volatile uint8_t s_req = 0;        // 要求カウンタ（取りこぼし軽減用）
static volatile uint32_t s_irq_seen = 0;  // デバッグ用：割り込み回数カウンタ
volatile uint8_t  g_meas_req = 0;
volatile uint32_t g_meas_irq = 0;

void MeasTimer_Init(TIM_HandleTypeDef *htim)
{
    s_htim = htim;
    s_req = 0;
    s_irq_seen = 0;
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
    if (!s_htim) return;
    if (htim->Instance == s_htim->Instance) {
        if (s_req < 5) s_req++;
        g_meas_req = s_req;
        g_meas_irq++;
        s_irq_seen++;     // ★ 追加：デバッグ用カウンタ
    }
}

// 消費側
bool MeasTimer_Consume(void)
{
    bool ret = false;
    __disable_irq();
    if (s_req) { s_req--; ret = true; }
    __enable_irq();
    g_meas_req = s_req;       // ★ミラー更新
    return ret;
}

uint8_t MeasTimer_Pending(void)
{
    return s_req;
}

// デバッグ用
uint32_t MeasTimer_IrqCount(void)
{
    return s_irq_seen;
}
