#include "adc_MCP3428.h"
#include <math.h>
#include <limits.h>
#include <string.h>

/* 内部状態 */
static I2C_HandleTypeDef* _hi2c;
static uint8_t _addr8;
static uint8_t _last_config;
static uint8_t _read_buffer[3];

/** 初期化（I2Cアドレスとハンドラを保存） */
bool MCP3428_adc_init(I2C_HandleTypeDef* hi2c, uint8_t addr7bit)
{
    _hi2c  = hi2c;
    _addr8 = (MCP3428_DEFAULT_ADDR | (addr7bit & 0x03)) << 1;

    // デバイス準備確認
    if (HAL_I2C_IsDeviceReady(_hi2c, _addr8, 3, 100) != HAL_OK) {
        return false;
    }

    return true;
}

/** コンフィグ送信：チャンネル、分解能、ゲインを指定し変換開始 */
bool MCP3428_start_conversion(uint8_t channel, MCP3428_Resolution res, MCP3428_Gain gain)
{
    if (channel < 1 || channel > 4) return false;

    uint8_t config = 0;
    config |= ((channel - 1) << 5);  // チャンネル
    config |= (1 << 4);              // One-shot
    switch (res) {
        case MCP3428_SPS_12BIT: config |= (0 << 2); break;
        case MCP3428_SPS_14BIT: config |= (1 << 2); break;
        case MCP3428_SPS_16BIT: config |= (2 << 2); break;
        default: return false;
    }

    switch (gain) {
        case MCP3428_GAIN_1: config |= 0; break;
        case MCP3428_GAIN_2: config |= 1; break;
        case MCP3428_GAIN_4: config |= 2; break;
        case MCP3428_GAIN_8: config |= 3; break;
        default: return false;
    }

    config |= 0x80;  // Start bit

    _last_config = config;

    if (HAL_I2C_Master_Transmit(_hi2c, _addr8, &config, 1, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    HAL_Delay(200);  // 最大変換時間待機（16bitで最大 133ms）
    return true;
}

/** コンバージョン完了確認（データ読み出し＆READYビット判定） */
bool MCP3428_check_conversion_complete(void)
{
    if (HAL_I2C_Master_Receive(_hi2c, _addr8, _read_buffer, 3, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    return (_read_buffer[2] & 0x80) == 0;  // READYビットが0なら完了
}

/** 生ADC値読み出し（符号付き） */
int16_t MCP3428_adc_read_raw(void)
{
    // READY チェック省略（事前に check_conversion_complete を推奨）
    if (HAL_I2C_Master_Receive(_hi2c, _addr8, _read_buffer, 3, HAL_MAX_DELAY) != HAL_OK) {
        return INT16_MIN;  // 異常値返す
    }

    int16_t raw = (_read_buffer[0] << 8) | _read_buffer[1];
    if (raw & 0x8000) raw -= 0x10000;
    return raw;
}

/** 電圧[mV]に変換（分圧・補正込み） */
int16_t MCP3428_adc_read_millivolt(void)
{
    int16_t raw = MCP3428_adc_read_raw();
    if (raw == INT16_MIN) return raw;

    float mv_f = raw * RESOLUTION_FACTOR_MV * ATTENUATION * GAIN_CALIB;

    // 四捨五入
    return (mv_f >= 0.0f) ? (int16_t)(mv_f + 0.5f) : (int16_t)(mv_f - 0.5f);
}
