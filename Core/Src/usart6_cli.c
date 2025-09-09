/*
 * usart6_cli.c
 *  Created on: Sep 8, 2025
 *      Author: ssshu
 */
#include "main.h"
#include "usart6_cli.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>              // strtoul, strtof
#include "adc_MCP3428.h"
#include "dac_MCP4728.h"
#include "dds_AD9833.h"
#include "mux_sn74lvc1g3157.h"
#include <stdio.h>

extern UART_HandleTypeDef huart6;

#ifndef CLI_RX_BUF_SZ
#define CLI_RX_BUF_SZ 256
#endif
#ifndef CLI_LINE_SZ
#define CLI_LINE_SZ   128
#endif

static volatile uint8_t  rx_byte;
static volatile uint8_t  rb[CLI_RX_BUF_SZ];
static volatile uint16_t r_head = 0, r_tail = 0;
static char line[CLI_LINE_SZ];

static float           g_volt   = 1.00f;   // 直近の設定電圧（V）
static bool            g_echo   = true;    // CMD=[...] を出すか

static void rb_push(uint8_t b){
  uint16_t n = (uint16_t)((r_head + 1) % CLI_RX_BUF_SZ);
  if (n != r_tail) { rb[r_head] = b; r_head = n; } // 溢れたら捨てる
}
static bool rb_pop(uint8_t* out){
  if (r_head == r_tail) return false;
  *out = rb[r_tail];
  r_tail = (uint16_t)((r_tail + 1) % CLI_RX_BUF_SZ);
  return true;
}

void CLI_Init(void){
  HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
}

void CLI_Write(const char* s){
  HAL_UART_Transmit(&huart6, (uint8_t*)s, (uint16_t)strlen(s), 100);
}

static void handle_command(const char* cmd);

// ★受信完了コールバック（必須）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart){
  if (huart->Instance == USART6){
    rb_push(rx_byte); // 受信1バイトをリングへ
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1); // 次を再開
  }
}
// （任意）エラー時も受信を再開
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart){
  if (huart->Instance == USART6){
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  }
}

void CLI_Poll(void){
  static size_t acc_len = 0;     // ★行の累積長（呼び出しまたぎで保持）
  uint8_t b;
  bool saw_eol = false;

  // リングから取り出し（CR/LFが先に来たら、先頭なら無視して継続）
  while (rb_pop(&b)) {
    if (b == '\r' || b == '\n') {
      if (acc_len == 0) {
        // 先頭の余分なEOL（例：前回のCRLFの片割れ）→ 捨てて次を見る
        continue;
      } else {
        saw_eol = true;
        break;
      }
    }
    if (acc_len + 1 < sizeof(line)) {
      line[acc_len++] = (char)b;
    }
  }

  if (!saw_eol) return;

  line[acc_len] = '\0';          // ここで1行完成！

  // 前後空白をtrim
  size_t s = 0, e = acc_len;
  while (s < e && isspace((unsigned char)line[s])) s++;
  while (e > s && isspace((unsigned char)line[e-1])) e--;

  // 大文字化（比較を楽に）
  for (size_t i = s; i < e; ++i) line[i] = (char)toupper((unsigned char)line[i]);
  line[e] = '\0';

  // デバッグ表示（ECHOで切替）
  if (g_echo) {
    CLI_Write("CMD=[");
    CLI_Write(&line[s]);
    CLI_Write("]\r\n");
  }

  // 実処理
  handle_command(&line[s]);

  // 次の行のためにリセット
  acc_len = 0;
}


static AD9833_WaveType g_wave   = AD9833_SINE;
static uint32_t        g_freqHz = 50;
static uint16_t        g_phase  = 0;


// ★コマンド処理（1か所だけ）
static void handle_command(const char* cmd){
  if (!cmd || !*cmd) return;

  if (strcmp(cmd, "PING") == 0) { CLI_Write("PONG\r\n"); return; }
  if (strcmp(cmd, "*IDN?") == 0) { CLI_Write("KOS21,SignalGen,0.1.0\r\n"); return; }

  // ---- 測定: 電圧読み取り（mV→V表示）----
  if (strcmp(cmd, "MEAS:VOLT?") == 0) {
    // CH4 / 16bit / ワンショット / ゲインx1 で読む（必要に応じて変更OK）
    MCP3428_SetConfig(&hadc3428,
                      MCP3428_CHANNEL_4,
                      MCP3428_RESOLUTION_16BIT,
                      MCP3428_MODE_ONESHOT,
                      MCP3428_GAIN_1X);
    int16_t mv = MCP3428_ReadMilliVolt(&hadc3428);
    if (mv == INT16_MIN) { CLI_Write("ERR:MEAS\r\n"); }
    else {
      char buf[24];
      // 小数3桁でV表示
      snprintf(buf, sizeof(buf), "%ld.%03ld\r\n",
               (long)(mv/1000), (long)labs((long)mv%1000));
      CLI_Write(buf);
    }
    return;
  }

  // ---- 出力: 周波数設定 ----
  // 例) "SOUR:FREQ 1000"
  if (strncmp(cmd, "SOUR:FREQ ", 10) == 0) {
    const char* p = cmd + 10;
    unsigned long hz = strtoul(p, NULL, 10);
    if (hz == 0 || hz > 1000000UL) { // 上限は暫定で1MHz（必要なら広げる）
      CLI_Write("ERR:RANGE\r\n");
    } else {
      g_freqHz = (uint32_t)hz;
      AD9833_Set(g_freqHz, g_wave, g_phase);
      CLI_Write("OK\r\n");
    }
    return;
  }

  // ---- 出力: 波形選択 ----
  // 例) "SOUR:FUNC SINE" / "SOUR:FUNC TRI" / "SOUR:FUNC SQU"
  if (strncmp(cmd, "SOUR:FUNC ", 10) == 0) {
    const char* p = cmd + 10;
    if      (strncmp(p, "SINE", 4) == 0) g_wave = AD9833_SINE;
    else if (strncmp(p, "TRI",  3) == 0) g_wave = AD9833_TRIANGLE;
    else if (strncmp(p, "SQU",  3) == 0) g_wave = AD9833_SQUARE;
    else { CLI_Write("ERR:ARG\r\n"); return; }
    AD9833_Set(g_freqHz, g_wave, g_phase);
    CLI_Write("OK\r\n");
    return;
  }

  // ---- 出力: DACで振幅（全CH同じ値に） ----
  // 例) "SOUR:VOLT 1.23"
  if (strncmp(cmd, "SOUR:VOLT ", 10) == 0) {
    const char* p = cmd + 10;
    float v = strtof(p, NULL);
    if (v < 0.0f) v = 0.0f;
    if (v > 2.048f) v = 2.048f;               // 内部Vref=2.048V, GAIN=1x 前提
    g_volt = v;
    uint16_t code = (uint16_t)((v * 4095.0f / 2.048f) + 0.5f);
    bool ok = true;
    ok &= MCP4728_SetChannel(MCP4728_CHANNEL_A, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
    ok &= MCP4728_SetChannel(MCP4728_CHANNEL_B, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
    ok &= MCP4728_SetChannel(MCP4728_CHANNEL_C, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
    ok &= MCP4728_SetChannel(MCP4728_CHANNEL_D, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
    CLI_Write(ok ? "OK\r\n" : "ERR:DAC\r\n");
    return;
  }

  // ---- DDS MCLKの切替（内部/外部） ----
  // 例) "CLK INT" / "CLK EXT"
  if (strncmp(cmd, "CLK ", 4) == 0) {
    const char* p = cmd + 4;
    if      (strncmp(p, "INT", 3) == 0) { CLK_MuxSet(Int_CLK_ToDDS); CLI_Write("OK\r\n"); }
    else if (strncmp(p, "EXT", 3) == 0) { CLK_MuxSet(Ext_CLK_ToDDS); CLI_Write("OK\r\n"); }
    else CLI_Write("ERR:ARG\r\n");
    return;
  }

  if (strcmp(cmd, "SOUR:FREQ?") == 0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%lu\r\n", (unsigned long)g_freqHz);
    CLI_Write(buf);
    return;
  }

  if (strcmp(cmd, "SOUR:FUNC?") == 0) {
    const char* name = (g_wave==AD9833_SINE)?"SINE":(g_wave==AD9833_TRIANGLE)?"TRI":"SQU";
    CLI_Write(name); CLI_Write("\r\n");
    return;
  }

  if (strcmp(cmd, "SOUR:VOLT?") == 0) {
    char buf[24];
    long mV = (long)(g_volt * 1000.0f + 0.5f);        // V→mV（四捨五入）
    snprintf(buf, sizeof(buf), "%ld.%03ld\r\n", mV/1000, labs(mV%1000));
    CLI_Write(buf);
    return;
  }


  // --- デバッグ出力の切替 ---
  if (strncmp(cmd, "ECHO ", 5) == 0) {
    const char* p = cmd + 5;
    if (strncmp(p, "ON", 2) == 0)  { g_echo = true;  CLI_Write("OK\r\n"); }
    else if (strncmp(p, "OFF", 3) == 0) { g_echo = false; CLI_Write("OK\r\n"); }
    else CLI_Write("ERR:ARG\r\n");
    return;
  }

  CLI_Write("ERR:UNKNOWN_CMD\r\n");
}
