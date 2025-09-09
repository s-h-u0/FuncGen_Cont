/*
 * usart6_cli.h
 *
 *  Created on: Sep 8, 2025
 *      Author: ssshu
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

void CLI_Init(void);        // 受信開始
void CLI_Poll(void);        // 1ms〜適当な頻度で呼ぶ
void CLI_Write(const char* s); // 送信ヘルパ
