#ifndef ADC_MCP3428_H
#define ADC_MCP3428_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// 7bit アドレス (<<1 して HAL に渡す)
#define MCP3428_DEFAULT_ADDR  0x68

typedef enum {
    MCP3428_CHANNEL_1 = 1,
    MCP3428_CHANNEL_2,
    MCP3428_CHANNEL_3,
    MCP3428_CHANNEL_4
} MCP3428_Channel_t;

typedef enum {
    MCP3428_RESOLUTION_12BIT = 12,
    MCP3428_RESOLUTION_14BIT = 14,
    MCP3428_RESOLUTION_16BIT = 16
} MCP3428_Resolution_t;

/// 変換モード：ワンショット／連続
typedef enum {
    MCP3428_MODE_ONESHOT = 0,
    MCP3428_MODE_CONTINUOUS
} MCP3428_Mode_t;

/// PGA ゲイン (1,2,4,8)
typedef enum {
    MCP3428_GAIN_1X = 1,
    MCP3428_GAIN_2X = 2,
    MCP3428_GAIN_4X = 4,
    MCP3428_GAIN_8X = 8
} MCP3428_Gain_t;

/// 内部ハンドル構造体
typedef struct {
    I2C_HandleTypeDef   *i2c;
    uint8_t              addr;     ///< HAL用8bitアドレス
    MCP3428_Channel_t    channel;
    MCP3428_Resolution_t res;
    MCP3428_Mode_t       mode;
    MCP3428_Gain_t       gain;
} MCP3428_HandleTypeDef;

/**
 * @brief MCP3428 初期化 (ACK チェック)
 */
bool MCP3428_Init(MCP3428_HandleTypeDef *dev,
                  I2C_HandleTypeDef      *hi2c,
                  uint8_t                 addr7bit);

/**
 * @brief チャンネル毎の設定を行い、変換を開始
 */
bool MCP3428_SetConfig(MCP3428_HandleTypeDef *dev,
                       MCP3428_Channel_t      channel,
                       MCP3428_Resolution_t   resolution,
                       MCP3428_Mode_t         mode,
                       MCP3428_Gain_t         gain);

/**
 * @brief データレジスタから ADC 値を取得（Ready ビット待ち含む）
 * @return signed 生データ (LSB 単位)、エラー時は INT32_MIN
 */
int32_t MCP3428_ReadADC(MCP3428_HandleTypeDef *dev);

/**
 * @brief 生データを mV 単位の整数に変換
 * @return mV 単位の signed 値、通信エラー時は INT16_MIN
 */
int16_t MCP3428_ReadMilliVolt(MCP3428_HandleTypeDef *dev);

#endif  // ADC_MCP3428_H
