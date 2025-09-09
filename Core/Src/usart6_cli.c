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
#include <stdlib.h>
#include <stdio.h>

#include "adc_MCP3428.h"
#include "dac_MCP4728.h"
#include "dds_AD9833.h"
#include "mux_sn74lvc1g3157.h"
#include "dpot_AD5292.h"

/* ====== 環境依存ハンドル ====== */
extern UART_HandleTypeDef huart6;
extern MCP3428_HandleTypeDef hadc3428;

/* ====== 調整可能な定義 ====== */
#ifndef CLI_RX_BUF_SZ
#define CLI_RX_BUF_SZ 256
#endif
#ifndef CLI_LINE_SZ
#define CLI_LINE_SZ   128
#endif
#ifndef VOLTAGE_MAX_V
#define VOLTAGE_MAX_V 40U  /* AD5292 目標電圧の上限(0..40V想定) */
#endif

/* ====== 受信リングバッファ ====== */
static volatile uint8_t  rx_byte;
static volatile uint8_t  rb[CLI_RX_BUF_SZ];
static volatile uint16_t r_head = 0, r_tail = 0;
static char line[CLI_LINE_SZ];

/* ====== 出力パラメータ（DDS/DAC/DPOT） ====== */
static AD9833_WaveType g_wave   = AD9833_SINE;
static uint32_t        g_freqHz = 50;
static uint16_t        g_phase  = 0;       /* 単位: 度(0..359) 想定 */
static float           g_volt   = 1.00f;   /* MCP4728 への電圧(0..2.048V) */
static uint32_t        g_potVolt= 0;       /* AD5292 目標電圧(0..VOLTAGE_MAX_V) */

/* ====== UI/CLI 共通状態 ====== */
static volatile uint8_t g_run_state = 0;   /* 0=STOP, 1=RUN */
static bool             g_echo      = true;

/* --- UIへ通知するための外部関数（MainView.cpp 側で定義） --- */
void ui_notify_runstop(int running);
void ui_set_desired_value(int which, uint32_t v);  // which: 0=Volt, 1=Phase

/* ----- 共通ユーティリティ ----- */
static void rb_push(uint8_t b){
  uint16_t n = (uint16_t)((r_head + 1u) % CLI_RX_BUF_SZ);
  if (n != r_tail) { rb[r_head] = b; r_head = n; } /* 溢れたら捨てる */
}
static bool rb_pop(uint8_t* out){
  if (r_head == r_tail) return false;
  *out = rb[r_tail];
  r_tail = (uint16_t)((r_tail + 1u) % CLI_RX_BUF_SZ);
  return true;
}

static void write_ok(void){ CLI_Write("OK\r\n"); }
static void write_err(const char* e){ CLI_Write(e); CLI_Write("\r\n"); }

/* DDS と DAC/DPOT を現在値で反映 */
static void apply_dds(void){
  AD9833_Set(g_freqHz, g_wave, g_phase);
}
static bool apply_dac_all(float v){
  if (v < 0.0f) v = 0.0f;
  if (v > 2.048f) v = 2.048f;  /* Vref=2.048V, GAIN=1x を想定 */
  g_volt = v;
  uint16_t code = (uint16_t)((v * 4095.0f / 2.048f) + 0.5f);
  bool ok = true;
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_A, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_B, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_C, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_D, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  return ok;
}
static void apply_run_on(void){
  apply_dds();
  AD5292_SetVoltage(g_potVolt); /* 出力系ON（POT目標へ） */
}
static void apply_run_off(void){
  /* 安全側：出力0Vへ */
  AD5292_SetVoltage(0);
}

/* ====== 公開関数 ====== */
void CLI_Init(void){
  HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
}

void CLI_Write(const char* s){
  if (!s) return;
  HAL_UART_Transmit(&huart6, (uint8_t*)s, (uint16_t)strlen(s), 100);
}

/* UI→CLIへ状態通知（共通で使える単一点） */
void CLI_SetRunState_FromUI(int on){
  uint8_t next = on ? 1u : 0u;
  if (g_run_state == next) return;
  g_run_state = next;
  if (g_run_state) apply_run_on(); else apply_run_off();
}

int CLI_GetRunState(void){ return (int)g_run_state; }

/* ====== UART コールバック（割込み受信） ====== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart){
  if (huart->Instance == USART6){
    rb_push(rx_byte);
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart){
  if (huart->Instance == USART6){
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  }
}

/* 前方宣言 */
static void handle_command(const char* cmd);

/* ====== ポーリング：1行単位で処理 ====== */
void CLI_Poll(void){
  static size_t acc_len = 0;
  uint8_t b;
  bool saw_eol = false;

  while (rb_pop(&b)) {
    if (b == '\r' || b == '\n') {
      if (acc_len == 0) { continue; } /* 空行EOLは捨てる */
      saw_eol = true;
      break;
    }
    if (acc_len + 1 < sizeof(line)) {
      line[acc_len++] = (char)b;
    }
  }
  if (!saw_eol) return;

  line[acc_len] = '\0';

  /* trim & 大文字化 */
  size_t s = 0, e = acc_len;
  while (s < e && isspace((unsigned char)line[s])) s++;
  while (e > s && isspace((unsigned char)line[e-1])) e--;

  for (size_t i = s; i < e; ++i) {
    line[i] = (char)toupper((unsigned char)line[i]);
  }
  line[e] = '\0';

  if (g_echo) {
    CLI_Write("CMD=["); CLI_Write(&line[s]); CLI_Write("]\r\n");
  }

  handle_command(&line[s]);
  acc_len = 0;
}

/* ====== コマンド処理本体 ====== */
static void handle_command(const char* cmd){
  if (!cmd || !*cmd) return;

  /* 汎用 */
  if (strcmp(cmd, "PING") == 0) { CLI_Write("PONG\r\n"); return; }
  if (strcmp(cmd, "*IDN?") == 0){ CLI_Write("KOS21,SignalGen,0.1.0\r\n"); return; }

  /* 測定: 電圧（例: CH4 / 16bit / ONESHOT / 1x）*/
  if (strcmp(cmd, "MEAS:VOLT?") == 0) {
    MCP3428_SetConfig(&hadc3428,
                      MCP3428_CHANNEL_4,
                      MCP3428_RESOLUTION_16BIT,
                      MCP3428_MODE_ONESHOT,
                      MCP3428_GAIN_1X);
    int16_t mv = MCP3428_ReadMilliVolt(&hadc3428);
    if (mv == INT16_MIN) { write_err("ERR:MEAS"); }
    else {
      char buf[24];
      snprintf(buf, sizeof(buf), "%ld.%03ld\r\n",
               (long)(mv/1000), (long)labs((long)mv%1000));
      CLI_Write(buf);
    }
    return;
  }

  /* 出力: 周波数 */
  if (strncmp(cmd, "SOUR:FREQ ", 10) == 0) {
    const char* p = cmd + 10;
    unsigned long hz = strtoul(p, NULL, 10);
    if (hz == 0 || hz > 1000000UL) { write_err("ERR:RANGE"); }
    else { g_freqHz = (uint32_t)hz; apply_dds(); write_ok(); }
    return;
  }
  if (strcmp(cmd, "SOUR:FREQ?") == 0) {
    char buf[24]; snprintf(buf, sizeof(buf), "%lu\r\n", (unsigned long)g_freqHz);
    CLI_Write(buf); return;
  }

  /* 出力: 波形 */
  if (strncmp(cmd, "SOUR:FUNC ", 10) == 0) {
    const char* p = cmd + 10;
    if      (strncmp(p, "SINE", 4) == 0) g_wave = AD9833_SINE;
    else if (strncmp(p, "TRI",  3) == 0) g_wave = AD9833_TRIANGLE;
    else if (strncmp(p, "SQU",  3) == 0) g_wave = AD9833_SQUARE;
    else { write_err("ERR:ARG"); return; }
    apply_dds(); write_ok(); return;
  }
  if (strcmp(cmd, "SOUR:FUNC?") == 0) {
    const char* name = (g_wave==AD9833_SINE)?"SINE":(g_wave==AD9833_TRIANGLE)?"TRI":"SQU";
    CLI_Write(name); CLI_Write("\r\n"); return;
  }

  /* 出力: 位相 */
  if (strncmp(cmd, "SOUR:PHAS ", 10) == 0) {
    const char* p = cmd + 10;
    unsigned long deg = strtoul(p, NULL, 10);
    if (deg > 359) deg = 359;
    g_phase = (uint16_t)deg;
    apply_dds();
    ui_set_desired_value(1, g_phase);   /* Phase=1 をUIへ通知 */
    write_ok(); return;
  }
  if (strcmp(cmd, "SOUR:PHAS?") == 0) {
    char buf[8]; snprintf(buf, sizeof(buf), "%u\r\n", (unsigned)g_phase);
    CLI_Write(buf); return;
  }

  /* 出力: DAC で振幅（全CH同値） */
  if (strncmp(cmd, "SOUR:VOLT ", 10) == 0) {
    const char* p = cmd + 10;
    float v = strtof(p, NULL);
    if (apply_dac_all(v)) write_ok(); else write_err("ERR:DAC");
    return;
  }
  if (strcmp(cmd, "SOUR:VOLT?") == 0) {
    char buf[24];
    long mV = (long)(g_volt * 1000.0f + 0.5f);
    snprintf(buf, sizeof(buf), "%ld.%03ld\r\n", mV/1000, labs(mV%1000));
    CLI_Write(buf); return;
  }

  /* DDS MCLK 切替 */
  if (strncmp(cmd, "CLK ", 4) == 0) {
    const char* p = cmd + 4;
    if      (strncmp(p, "INT", 3) == 0) { CLK_MuxSet(Int_CLK_ToDDS); write_ok(); }
    else if (strncmp(p, "EXT", 3) == 0) { CLK_MuxSet(Ext_CLK_ToDDS); write_ok(); }
    else write_err("ERR:ARG");
    return;
  }

  /* デバッグ出力の切替 */
  if (strncmp(cmd, "ECHO ", 5) == 0) {
    const char* p = cmd + 5;
    if      (strncmp(p, "ON",  2) == 0) { g_echo = true;  write_ok(); }
    else if (strncmp(p, "OFF", 3) == 0) { g_echo = false; write_ok(); }
    else write_err("ERR:ARG");
    return;
  }

  /* AD5292: 目標電圧（RUN用） */
  if (strncmp(cmd, "POT:VOLT ", 9) == 0) {
    const char* p = cmd + 9;
    unsigned long v = strtoul(p, NULL, 10);
    if (v > VOLTAGE_MAX_V) v = VOLTAGE_MAX_V;   /* 0..40V にクランプ */
    g_potVolt = (uint32_t)v;                    /* 保存 */
    if (g_run_state) {                          /* ← 修正: g_running ではなく g_run_state */
      AD5292_SetVoltage(g_potVolt);             /* 走行中はライブ反映 */
    }
    ui_set_desired_value(0, g_potVolt);         /* Voltage=0 をUIへ通知 */
    write_ok(); return;
  }
  if (strcmp(cmd, "POT:VOLT?") == 0) {
    char buf[12]; snprintf(buf, sizeof(buf), "%lu\r\n", (unsigned long)g_potVolt);
    CLI_Write(buf); return;
  }

  /* RUN / STOP */
  if (strcmp(cmd, "RUN") == 0) {
    if (!g_run_state) {
      g_run_state = 1;
      apply_run_on();
      ui_notify_runstop(1);
    }
    write_ok(); return;
  }
  if (strcmp(cmd, "STOP") == 0) {
    if (g_run_state) {
      g_run_state = 0;
      AD5292_Set(0x400);
      ui_notify_runstop(0);
    }
    write_ok(); return;
  }
  if (strcmp(cmd, "RUN?") == 0) {
    char buf[8]; snprintf(buf, sizeof(buf), "%u\r\n", (unsigned)g_run_state);
    CLI_Write(buf); return;
  }

  /* 未定義 */
  write_err("ERR:UNKNOWN_CMD");
}
