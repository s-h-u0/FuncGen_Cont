// adc_MCP3428.c

#include "adc_MCP3428.h"
#include <math.h>
#include <limits.h>

// --------------------------------------------------------
// 内部：設定ビットを組み立て
// --------------------------------------------------------
static inline uint8_t _build_config(MCP3428_HandleTypeDef *dev) {
    uint8_t cfg = 0;
    // チャンネル（0b00〜0b11）
    cfg |= ((uint8_t)dev->channel - 1) & 0x03;
    // モード (連続なら1)
    cfg |= (dev->mode == MCP3428_MODE_CONTINUOUS ? 1 : 0) << 2;
    // 解像度ビット [(12→0),(14→1),(16→2)]
    cfg |= (((dev->res - 12) / 2) & 0x03) << 3;
    // PGAゲインの log2
    {
        float lg = log2f((float)dev->gain);
        uint8_t g = (uint8_t)(lg + 0.5f);
        cfg |= (g & 0x03) << 5;
    }
    // 変換開始ビット
    cfg |= 1 << 7;
    return cfg;
}

// --------------------------------------------------------
// 初期化＋ ACK 確認
// --------------------------------------------------------
bool MCP3428_Init(MCP3428_HandleTypeDef *dev,
                  I2C_HandleTypeDef      *hi2c,
                  uint8_t                 addr7bit)
{
    dev->i2c  = hi2c;
    dev->addr = addr7bit << 1; // HAL 用 8bit アドレス
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
    uint8_t cfg = _build_config(dev);
    return (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &cfg, 1, 200) == HAL_OK);
}

// --------------------------------------------------------
// 生ADC値読み出し (−32768～+32767 を返す)
// --------------------------------------------------------
int32_t MCP3428_ReadADC(MCP3428_HandleTypeDef *dev)
{
    uint8_t buf[3];
    uint8_t conv_reg = 0x00;  // コンバージョンレジスタアドレス

    // ワンショットモードなら変換開始コマンドを送信
    if (dev->mode == MCP3428_MODE_ONESHOT) {
        uint8_t cfg = _build_config(dev);
        if (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &cfg, 1, HAL_MAX_DELAY) != HAL_OK) {
            return INT32_MIN;
        }
    }
    // 連続モードでは MCP3428_SetConfig() 側で開始済みとする

    // RDY ビットが0になるまでポーリングしつつ、必ず先にレジスタポインタを送る
    do {
        if (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &conv_reg, 1, HAL_MAX_DELAY) != HAL_OK) {
            return INT32_MIN;
        }
        if (HAL_I2C_Master_Receive(dev->i2c, dev->addr, buf, 3, HAL_MAX_DELAY) != HAL_OK) {
            return INT32_MIN;
        }
    } while (buf[2] & 0x80);

    // 生データを展開（符号拡張含む）
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
#define MCP3428_RES_MV    (0.0625f)   // LSB あたりの mV (16bit, 15 SPS)
#define MCP3428_ATT       (11.0f)     // 分圧比
#define MCP3428_GCALIB    (0.998f)    // ゲイン補正

int16_t MCP3428_ReadMilliVolt(MCP3428_HandleTypeDef *dev)
{
    int32_t raw = MCP3428_ReadADC(dev);
    if (raw == INT32_MIN) {
        return INT16_MIN;
    }
    float mv_f = raw * (MCP3428_ATT * MCP3428_RES_MV) * MCP3428_GCALIB;
    return (int16_t)mv_f;
}
