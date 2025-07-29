// adc_MCP3428.c
#include "adc_MCP3428.h"
#include <math.h>
#include <limits.h>

// --------------------------------------------------------
// 内部：設定ビットを組み立て
// --------------------------------------------------------
static inline uint8_t _build_config(MCP3428_HandleTypeDef *dev)
{
    /* Bit layout (MSB→LSB):
       b7: RDY/Start  | b6–5: Channel  | b4: Mode   | b3–2: Sample rate | b1–0: PGA gain */
    uint8_t cfg = 0b00000000;

    // 1) Start conversion / RDY flag (bit7)
    cfg |= 0b10000000;  // 1 << 7

    // 2) Channel selection (bits6–5)
    //    00=Ch1, 01=Ch2, 10=Ch3, 11=Ch4
    uint8_t ch_bits = ((uint8_t)dev->channel & 0x03) << 5;
    cfg |= ch_bits;

    // 3) Mode (bit4): 0=One-Shot, 1=Continuous
    uint8_t mode_bit = (dev->mode == MCP3428_MODE_CONTINUOUS) ? 0b00010000 : 0b00000000;
    cfg |= mode_bit;

    // 4) Sample rate (bits3–2): 00=12bit, 01=14bit, 10=16bit
    uint8_t sr_code = 0;
    if (dev->res == MCP3428_RESOLUTION_14BIT)      sr_code = 0b00000100;
    else if (dev->res == MCP3428_RESOLUTION_16BIT) sr_code = 0b00001000;
    cfg |= sr_code;

    // 5) PGA gain (bits1–0): 00=×1, 01=×2, 10=×4, 11=×8
    uint8_t gain_code = 0;
    switch (dev->gain) {
        case MCP3428_GAIN_2X: gain_code = 0b01; break;
        case MCP3428_GAIN_4X: gain_code = 0b10; break;
        case MCP3428_GAIN_8X: gain_code = 0b11; break;
        default:              gain_code = 0b00; break;  // ×1
    }
    cfg |= gain_code;

    return cfg;
}

// --------------------------------------------------------
// 初期化＋ ACK 確認
// --------------------------------------------------------
bool MCP3428_Init(MCP3428_HandleTypeDef *dev,
                  I2C_HandleTypeDef      *hi2c)
{
    dev->i2c  = hi2c;
    dev->addr = MCP3428_DEFAULT_ADDR << 1; // HAL用8bitアドレス
    return (HAL_I2C_IsDeviceReady(dev->i2c, dev->addr, 3, 100) == HAL_OK);
}

// --------------------------------------------------------
// コンフィグ送信（チャンネル／分解能／モード／ゲイン設定）
// --------------------------------------------------------
bool MCP3428_SetConfig(MCP3428_HandleTypeDef *dev,
                       MCP3428_Channel_t      channel,
                       MCP3428_Resolution_t   resolution,
                       MCP3428_Mode_t         mode,
                       MCP3428_Gain_t         gain)
{
    dev->channel = channel;
    dev->res     = resolution;
    dev->mode    = mode;
    dev->gain    = gain;
    uint8_t cfg  = _build_config(dev);
    return (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &cfg, 1, 200) == HAL_OK);
}

// --------------------------------------------------------
// 生ADC値読み出し (−32768～+32767 を返す)
// --------------------------------------------------------
int32_t MCP3428_ReadADC(MCP3428_HandleTypeDef *dev)
{
    uint8_t buf[3];
    uint8_t conv_reg = 0x00;

    // ワンショットモードなら変換開始コマンドを送信
    if (dev->mode == MCP3428_MODE_ONESHOT) {
        uint8_t cfg = _build_config(dev);
        if (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &cfg, 1, HAL_MAX_DELAY) != HAL_OK) {
            return INT32_MIN;
        }
    }

    // RDY ビットが0になるまでポーリング
    do {
        if (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &conv_reg, 1, HAL_MAX_DELAY) != HAL_OK) {
            return INT32_MIN;
        }
        if (HAL_I2C_Master_Receive(dev->i2c, dev->addr, buf, 3, HAL_MAX_DELAY) != HAL_OK) {
            return INT32_MIN;
        }
    } while (buf[2] & 0x80);

    // 生データ展開（符号拡張含む）
    int32_t raw;
    switch (dev->res) {
        case MCP3428_RESOLUTION_12BIT:
            raw = ((buf[0] & 0x0F) << 8) | buf[1];
            if (raw & 0x0800) raw -= 0x1000;
            break;
        case MCP3428_RESOLUTION_14BIT:
            raw = ((buf[0] & 0x3F) << 8) | buf[1];
            if (raw & 0x2000) raw -= 0x4000;
            break;
        case MCP3428_RESOLUTION_16BIT:
            raw = (int16_t)((buf[0] << 8) | buf[1]);
            break;
        default:
            return INT32_MIN;
    }
    return raw;
}

// --------------------------------------------------------
// 生データ→mV変換ユーティリティ
// --------------------------------------------------------
#define MCP3428_RES_MV    (0.0625f)
#define MCP3428_ATT       (11.0f)
#define MCP3428_GCALIB    (0.998f)

int16_t MCP3428_ReadMilliVolt(MCP3428_HandleTypeDef *dev)
{
    int32_t raw = MCP3428_ReadADC(dev);
    if (raw == INT32_MIN) {
        return INT16_MIN;
    }
    float mv_f = raw * (MCP3428_ATT * MCP3428_RES_MV) * MCP3428_GCALIB;
    return (int16_t)mv_f;
}
