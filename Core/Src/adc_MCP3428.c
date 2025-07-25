#include "adc_MCP3428.h"
#include <math.h>

static inline uint8_t _build_config(MCP3428_HandleTypeDef *dev) {
    uint8_t cfg = 0;
    // チャンネル（0〜3）をセット
    cfg |= (uint8_t)(dev->channel - 1) & 0x03;
    // モード
    cfg |= (dev->mode == MCP3428_MODE_CONTINUOUS ? 1 : 0) << 2;
    // 解像度ビット [(12→0),(14→1),(16→2)]
    cfg |= (((dev->res - 12) / 2) & 0x03) << 3;
    // PGAゲインログ2
    uint8_t g = (uint8_t)(log2(dev->gain) + 0.5f);
    cfg |= (g & 0x03) << 5;
    // 最上位ビットで変換開始
    cfg |= 1 << 7;
    return cfg;
}

bool MCP3428_Init(MCP3428_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7bit) {
    dev->i2c  = hi2c;
    dev->addr = addr7bit << 1;
    // ACK 確認のみ
    return (HAL_I2C_IsDeviceReady(dev->i2c, dev->addr, 3, 100) == HAL_OK);
}

bool MCP3428_SetConfig(MCP3428_HandleTypeDef *dev,
                       MCP3428_Channel_t channel,
                       MCP3428_Resolution_t resolution,
                       MCP3428_Mode_t mode,
                       MCP3428_Gain_t gain) {
    dev->channel = channel;
    dev->res     = resolution;
    dev->mode    = mode;
    dev->gain    = gain;
    uint8_t cfg = _build_config(dev);
    return (HAL_I2C_Master_Transmit(dev->i2c, dev->addr, &cfg, 1, 200) == HAL_OK);
}

int32_t MCP3428_ReadADC(MCP3428_HandleTypeDef *dev) {
    uint8_t buf[3];
    // RDY ビットが 1→0 になるまでポーリング
    do {
        if (HAL_I2C_Master_Receive(dev->i2c, dev->addr, buf, 3, 200) != HAL_OK) {
            return INT32_MIN;  // 通信エラー
        }
    } while ((buf[2] & 0x80) != 0);

    int32_t raw = 0;
    switch (dev->res) {
        case MCP3428_RESOLUTION_12BIT:
            raw = ((buf[0] & 0x0F) << 8) | buf[1];
            if (raw & 0x800) raw -= 0x1000;
            break;
        case MCP3428_RESOLUTION_14BIT:
            raw = ((buf[0] & 0x3F) << 8) | buf[1];
            if (raw & 0x2000) raw -= 0x4000;
            break;
        case MCP3428_RESOLUTION_16BIT:
            raw = ((int16_t)((buf[0] << 8) | buf[1]));
            break;
        default:
            return INT32_MIN;
    }
    return raw;
}
