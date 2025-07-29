#include "adc_MCP3428.h"
#include <math.h>
#include <limits.h>
#include "stdbool.h"

/* ---- 定数 ---- */
/** 16bit 15sps, PGA×1 のコンフィグバイト */
static const uint8_t ADC_CFG = 0x98;

/** 1LSB あたりの電圧[mV]: 2.048V/32768*1000 */
#define RESOLUTION_FACTOR_MV  (2.048f / 32768.0f * 1000.0f)
/** 分圧比 */
#define ATTENUATION           (11.0f)
/** ゲイン補正 */
#define GAIN_CALIB            (0.998f)
/** MCP3428 の 7bit I2C アドレス */
#define MCP3428_DEFAULT_ADDR  0x68

/* ---- 内部変数 ---- */
static I2C_HandleTypeDef* _hi2c;
static uint8_t            _addr8;

/** 初期化：コンフィグ送信＆立ち上がり待ち */
bool MCP3428_adc_init(I2C_HandleTypeDef* hi2c, uint8_t addr7bit)
{
    _hi2c  = hi2c;
    _addr8 = addr7bit << 1;
/*
    // デバイスの ACK 確認（3回リトライ，100ms タイムアウト）
    if (HAL_I2C_IsDeviceReady(_hi2c, _addr8, 3, 100) != HAL_OK) {
        // そもそもデバイスが見つからない
        return false;
    }

*/

    // コンフィグ開始ビット送信
    if (HAL_I2C_Master_Transmit(_hi2c, _addr8, (uint8_t*)&ADC_CFG, 1, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    HAL_Delay(200);  // コンバージョン開始後の立ち上がり待ち
    return true;
}

/** 生データ読み出し：2バイト、符号付き結合 */
int16_t MCP3428_adc_read_raw(void)
{
    uint8_t buf[2];
    // READY ビットチェックなど省略 → すぐ 2 バイト読み
    HAL_I2C_Master_Receive(_hi2c, _addr8, buf, 2, HAL_MAX_DELAY);
    // ビッグエンディアン結合、符号付き
    return (int16_t)((buf[0] << 8) | buf[1]);
}

/** mV に変換して返却（四捨五入あり） */
int16_t MCP3428_adc_read_millivolt(void)
{
    int16_t raw = MCP3428_adc_read_raw();
    // 生値×分圧×1LSB[mV]×補正
    float mv_f = raw * (ATTENUATION * RESOLUTION_FACTOR_MV) * GAIN_CALIB;
    // 四捨五入
    if (mv_f >= 0.0f) mv_f += 0.5f;
    else             mv_f -= 0.5f;
    return (int16_t)mv_f;
}
