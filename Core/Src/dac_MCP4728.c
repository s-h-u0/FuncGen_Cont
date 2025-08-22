/*
 * dac_MCP4728.c
 *
 *  Created on: Jul 25, 2025
 *      Author: ssshu
 */


#include "dac_MCP4728.h"

#define CMD_MULTI_WRITE   0x40
#define CMD_MULTI_EEPROM  0x50

static I2C_HandleTypeDef *mcp4728_i2c = NULL;

bool MCP4728_Init(I2C_HandleTypeDef *hi2c)
{
    mcp4728_i2c = hi2c;

    return (HAL_I2C_IsDeviceReady(mcp4728_i2c, MCP4728_I2C_ADDR_DEFAULT, 3, 100) == HAL_OK);
}

#include "dac_MCP4728.h"  // 必要に応じて

bool MCP4728_SetChannel(MCP4728_Channel ch, uint16_t value,
                        MCP4728_Vref vref, MCP4728_Gain gain,
                        MCP4728_PowerDown pd, bool use_udac)
{
    if (value > 0x0FFF) {
        value = 0x0FFF;  // 安全な範囲に制限（12bit）
    }

    // コマンドバイトの生成（MULTI WRITE + チャネル + UDAC）
    uint8_t cmd = 0x40 | (ch << 1) | (use_udac ? 0x01 : 0x00);

    // 上位構成ビットを合成（Vref, PowerDown, Gain）+ 下位12bit値
    uint16_t reg = 0;
    reg |= ((uint16_t)vref << 15);       // bit15: VREF (1 = internal)
    reg |= ((uint16_t)pd   << 13);       // bit14-13: PD1, PD0
    reg |= ((uint16_t)gain << 12);       // bit12: Gain
    reg |= (value & 0x0FFF);             // bit11-0: 12bit DAC値

    // 送信バッファ作成
    uint8_t buf[3];
    buf[0] = cmd;
    buf[1] = (uint8_t)(reg >> 8);
    buf[2] = (uint8_t)(reg & 0xFF);

    // I2C書き込み実行
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(mcp4728_i2c,
                                                   MCP4728_I2C_ADDR_DEFAULT,
                                                   buf, 3, 100);
    return (st == HAL_OK);
}


bool MCP4728_FastWrite(uint16_t valA, uint16_t valB,
                       uint16_t valC, uint16_t valD)
{
    uint8_t buf[8];

    buf[0] = (uint8_t)(valA >> 8);
    buf[1] = (uint8_t)(valA & 0xFF);
    buf[2] = (uint8_t)(valB >> 8);
    buf[3] = (uint8_t)(valB & 0xFF);
    buf[4] = (uint8_t)(valC >> 8);
    buf[5] = (uint8_t)(valC & 0xFF);
    buf[6] = (uint8_t)(valD >> 8);
    buf[7] = (uint8_t)(valD & 0xFF);

    if (HAL_I2C_Master_Transmit(mcp4728_i2c, MCP4728_I2C_ADDR_DEFAULT, buf, 8, 100) != HAL_OK) {
        return false;
    }

    return true;
}

bool MCP4728_SaveToEEPROM(void)
{
    uint8_t readbuf[24];
    uint8_t writebuf[9];

    if (HAL_I2C_Master_Receive(mcp4728_i2c, MCP4728_I2C_ADDR_DEFAULT, readbuf, 24, 100) != HAL_OK) {
        return false;
    }

    writebuf[0] = CMD_MULTI_EEPROM | (MCP4728_CHANNEL_A << 1);
    writebuf[1] = readbuf[1];   writebuf[2] = readbuf[2];
    writebuf[3] = readbuf[7];   writebuf[4] = readbuf[8];
    writebuf[5] = readbuf[13];  writebuf[6] = readbuf[14];
    writebuf[7] = readbuf[19];  writebuf[8] = readbuf[20];

    if (HAL_I2C_Master_Transmit(mcp4728_i2c, MCP4728_I2C_ADDR_DEFAULT, writebuf, 9, 100) != HAL_OK) {
        return false;
    }

    HAL_Delay(15);  // 書き込み完了待ち

    return true;
}


