/*
 * usart6_cli.c
 *  Created on: Sep 8, 2025
 *      Author: ssshu
 *
 * ================================================================
 * モジュール概要
 * - USART6 を使ったシリアル CLI の実装。
 * - 受信は割り込み (HAL_UART_Receive_IT) で 1 バイトずつ取得。
 *   → 受信 ISR(HAL_UART_RxCpltCallback) でリングバッファへ格納し、
 *      次の 1 バイト受信を「再アーム」するだけ（ISR は最小限）。
 * - メインループから CLI_Poll() を呼ぶと、
 *   リングから取り出して「行（CR/LF 終端）」に組み立て、
 *   大文字化やトリムを行った上で handle_command() に渡す。
 * - handle_command() はデバイス制御（DDS/DAC/DPOT/ADC など）と
 *   UI への通知（ui_notify_runstop / ui_set_desired_value）を行う。
 * - RUN/STOP の統一制御口として CLI_SetRunState_FromUI() を用意。
 *
 * 設計の肝
 * - ISR では「短く・軽く」（バイトを押し込み＆再アームのみ）。
 * - 重い処理（行の解釈・コマンド実行・I2C/SPI アクセス等）は
 *   UI/メインスレッド側で逐次処理（スレッド安全）。
 * - リングバッファは「満杯なら新規バイトを捨てる」挙動。
 * ================================================================
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

#include "dipsw_221AMA16R.h"

/* ====== 環境依存ハンドル ======
 * - ボード側で定義されるハンドルを参照（extern）
 * - 本モジュールはそれらを直接用いる
 */
extern UART_HandleTypeDef huart6;
extern MCP3428_HandleTypeDef hadc3428;

/* ====== 調整可能な定義 ======
 * CLI_RX_BUF_SZ: 受信リングのバイト数
 * CLI_LINE_SZ  : 1 行の最大長（終端含む）
 * VOLTAGE_MAX_V: DPOT で扱う目標電圧の上限値（単位: V）
 */
#ifndef CLI_RX_BUF_SZ
#define CLI_RX_BUF_SZ 256
#endif
#ifndef CLI_LINE_SZ
#define CLI_LINE_SZ   128
#endif
#ifndef VOLTAGE_MAX_V
#define VOLTAGE_MAX_V 40U  /* AD5292 目標電圧の上限(0..40V想定) */
#endif

static int  addr_match_and_strip(char* cmd);
static bool is_query_cmd(const char* s);

static bool g_silent_reply = false;  // ブロードキャスト時に応答しない


static uint8_t g_addr = 0x0;   // 自局アドレス(0..15)

static void delay_us(uint32_t us){
  uint32_t cycles = (SystemCoreClock / 1000000U) * us;
  while (cycles--) { __NOP(); }
}
static inline uint32_t turnaround_us(void){
  uint32_t baud = huart6.Init.BaudRate ? huart6.Init.BaudRate : 115200U;
  return (12U * 1000000U) / baud + 200U; // 1文字(≈10–12bit) + 余裕
}

/* ====== 受信リングバッファ ======
 * rx_byte : 受信 1 バイト用の作業変数（ISR が書き込む / 再アーム時に指定）
 * rb[]    : 受信したデータの循環キュー（ISR→メイン間の受け渡し）
 * r_head  : 書き込み位置（ISR 側が前進）
 * r_tail  : 読み出し位置（メイン側が前進）
 * line[]  : 1 行の作業バッファ（CLI_Poll 内で使用）
 */
static volatile uint8_t  rx_byte;
static volatile uint8_t  rb[CLI_RX_BUF_SZ];
static volatile uint16_t r_head = 0, r_tail = 0;
static char line[CLI_LINE_SZ];

/* ====== 出力パラメータ（DDS/DAC/DPOT） ======
 * g_wave   : DDS 出力波形
 * g_freqHz : DDS 出力周波数
 * g_phase  : DDS 出力位相（0..359 度）
 * g_volt   : DAC(MCP4728) へ与える電圧（Vref=2.048V, GAIN=1x 前提）
 * g_potVolt: DPOT(AD5292) で実現する目標電圧（RUN 時に使用）
 */
static AD9833_WaveType g_wave   = AD9833_SINE;
static uint32_t        g_freqHz = 50;
static uint16_t        g_phase  = 0;       /* 単位: 度(0..359) 想定 */
static float           g_volt   = 1.00f;   /* MCP4728 への電圧(0..2.048V) */
static uint32_t        g_potVolt= 0;       /* AD5292 目標電圧(0..VOLTAGE_MAX_V) */

/* ====== UI/CLI 共通状態 ======
 * g_run_state: 出力の走行状態（0=STOP, 1=RUN）
 * g_echo     : 受信コマンドのエコー出力フラグ
 */
static volatile uint8_t g_run_state = 0;   /* 0=STOP, 1=RUN */
static bool             g_echo      = false;

/* --- UIへ通知するための外部関数（MainView.cpp 側で定義） ---
 * ui_notify_runstop(running): RUN/STOP の変更通知
 * ui_set_desired_value(which, v): 希望値の変更通知 (which: 0=Volt, 1=Phase)
 */
void ui_notify_runstop(int running);
void ui_set_desired_value(int which, uint32_t v);  // which: 0=Volt, 1=Phase

// RS-485 EN制御（PB11 = L_Reci_H_Send）: 0=受信, 1=送信
static inline void RS485_RXmode(void){
  HAL_GPIO_WritePin(L_Reci_H_Send_GPIO_Port, L_Reci_H_Send_Pin, GPIO_PIN_RESET);
}
static inline void RS485_TXmode(void){
  HAL_GPIO_WritePin(L_Reci_H_Send_GPIO_Port, L_Reci_H_Send_Pin, GPIO_PIN_SET);
}
static inline void UART6_WaitTxComplete(void){
  while (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_TC) == RESET) { /* wait */ }
}


/* ----- 共通ユーティリティ -------------------------------------
 * rb_push: 受信 1 バイトをリングへ格納（満杯なら捨てる）
 * rb_pop : リングから 1 バイト取り出し（空なら false）
 * write_ok / write_err: 定型応答
 * --------------------------------------------------------------*/
static void rb_push(uint8_t b){
  /* 次の書き込み位置（末端なら 0 に巻き戻し）を計算 */
  uint16_t n = (uint16_t)((r_head + 1u) % CLI_RX_BUF_SZ);
  /* 満杯判定：次位置が tail に追いつくなら捨てる（ブロックしない設計） */
  if (n != r_tail) { rb[r_head] = b; r_head = n; } /* 溢れたら捨てる */
}
static bool rb_pop(uint8_t* out){
  /* 空判定：head==tail ならデータなし */
  if (r_head == r_tail) return false;
  *out = rb[r_tail];
  /* 読み出し位置を 1 つ前進（末端なら 0 に巻き戻し） */
  r_tail = (uint16_t)((r_tail + 1u) % CLI_RX_BUF_SZ);
  return true;
}

static void write_ok(void){ if (!g_silent_reply) CLI_Write("OK\r\n"); }
static void write_err(const char* e){ if (!g_silent_reply){ CLI_Write(e); CLI_Write("\r\n"); } }

/* ====== DDS/DAC/DPOT 反映系ユーティリティ ======================
 * - apply_dds    : 現在の g_freqHz/g_wave/g_phase を DDS へ適用
 * - apply_dac_all: DAC(MCP4728) の全 CH を同じ電圧に設定（範囲クランプあり）
 * - apply_run_on : RUN 開始時の一括反映（DDS 適用→DPOT 目標電圧）
 * - apply_run_off: RUN 停止時の安全処理（DPOT 0V 相当へ）
 * ===============================================================*/
static void apply_dds(void){
  /* DDS の周波数/波形/位相を現在値で適用 */
  AD9833_Set(g_freqHz, g_wave, g_phase);
}
static bool apply_dac_all(float v){
  /* 範囲外ガード（Vref=2.048V, GAIN=1x を前提） */
  if (v < 0.0f) v = 0.0f;
  if (v > 2.048f) v = 2.048f;  /* Vref=2.048V, GAIN=1x を想定 */
  g_volt = v;
  /* 電圧→12bit コードへ変換（丸め） */
  uint16_t code = (uint16_t)((v * 4095.0f / 2.048f) + 0.5f);
  /* 4CH を同一設定（UDAC=true: 反映） */
  bool ok = true;
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_A, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_B, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_C, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  ok &= MCP4728_SetChannel(MCP4728_CHANNEL_D, code, MCP4728_VREF_INTERNAL, MCP4728_GAIN_1X, MCP4728_PD_NORMAL, true);
  return ok;
}
static void apply_run_on(void){
  /* RUN 時：DDS を現在値で有効にし、DPOT へ目標電圧を反映 */
  apply_dds();
  AD5292_SetVoltage(g_potVolt); /* 出力系ON（POT目標へ） */
}
static void apply_run_off(void){
  /* STOP 時：安全側の 0V へ（出力落とし） */
  AD5292_SetVoltage(0);
}

/* ====== 公開関数 ===============================================
 * CLI_Init          : UART 受信割り込みを 1 バイトでアーム
 * CLI_Write         : 同期送信（簡易）
 * CLI_SetRunState_FromUI : UI からの RUN/STOP 指示を一括反映
 * CLI_GetRunState   : RUN 状態の参照
 * ===============================================================*/
void CLI_Init(void){
  g_addr = (uint8_t)(DIP221_Read() & 0x0F);   // 0..F
  RS485_RXmode();
  HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
}



void CLI_Write(const char* s){
  if (!s) return;
  RS485_TXmode();
  HAL_UART_Transmit(&huart6, (uint8_t*)s, (uint16_t)strlen(s), 100);
  UART6_WaitTxComplete();
  delay_us(turnaround_us());     // ← これでOK（extern不要）
  RS485_RXmode();
}



/* UI→CLIへ状態通知（共通で使える単一点）
 * - on!=current なら状態変更し、RUN/STOP に応じて apply_* を呼ぶ。
 * - UI 側からはこの関数だけ呼べば安全手順が担保される。
 */
void CLI_SetRunState_FromUI(int on){
  uint8_t next = on ? 1u : 0u;
  if (g_run_state == next) return;
  g_run_state = next;
  if (g_run_state) apply_run_on(); else apply_run_off();
}

int CLI_GetRunState(void){ return (int)g_run_state; }

/* ====== UART コールバック（割込み受信） ========================
 * - HAL_UART_RxCpltCallback: 受信 1 バイト完了 ISR。リングへ積み、次を再アーム。
 * - HAL_UART_ErrorCallback : エラー時も受信を継続できるよう再アーム。
 *   ※ ISR では「短く・軽く」を徹底（重い処理はしない）
 * ===============================================================*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart){
  if (huart->Instance == USART6){
    /* 受信した 1 バイトをリングへ格納（満杯なら捨てる） */
    rb_push(rx_byte);
    /* 次の 1 バイト受信をすぐ再アーム（連続受信） */
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart){
  if (huart->Instance == USART6){
    /* エラー後も受信を継続するため再アーム */
    HAL_UART_Receive_IT(&huart6, (uint8_t*)&rx_byte, 1);
  }
}

/* 前方宣言（コマンド処理本体） */
static void handle_command(const char* cmd);

/* ====== ポーリング：1行単位で処理 =============================
 * CLI_Poll:
 * - リングからバイトを取り出し、CR/LF が来るまで line[] に蓄積。
 * - 改行が来たら行確定 → 先頭/末尾トリム → 大文字化。
 * - ECHO 出力（任意）後、handle_command() に渡す。
 * - 非ブロッキング：データがなければ即 return。
 * ===============================================================*/
void CLI_Poll(void){
  static size_t acc_len = 0;   /* line[] への格納済みバイト数（継続用） */
  uint8_t b;
  bool saw_eol = false;

  /* --- リングから取り出し、EOL(CR/LF) まで蓄積 --- */
  while (rb_pop(&b)) {
    if (b == '\r' || b == '\n') {
      if (acc_len == 0) { continue; } /* 空行EOLは捨てる（CR+LF の 2 文字目など） */
      saw_eol = true;
      break;
    }
    if (acc_len + 1 < sizeof(line)) {
      line[acc_len++] = (char)b;
    }
    /* バッファ満杯時は超過分を無視（静かに切り捨て） */
  }
  if (!saw_eol) return;  /* 行未完成なら終了（次回へ） */

  line[acc_len] = '\0';

  /* --- トリム（先頭/末尾の空白削除）＆大文字化 --- */
  size_t s = 0, e = acc_len;
  while (s < e && isspace((unsigned char)line[s])) s++;
  while (e > s && isspace((unsigned char)line[e-1])) e--;

  for (size_t i = s; i < e; ++i) {
    line[i] = (char)toupper((unsigned char)line[i]);
  }
  line[e] = '\0';



  int am = addr_match_and_strip(&line[s]);  // 1=自宛, 0=他宛無視, -1=ブロードキャスト/宛先無し
  if (am == 0) { acc_len = 0; return; }
  bool is_q = is_query_cmd(&line[s]);
  if (am < 0 && is_q) { acc_len = 0; return; }   // ブロードキャスト問い合わせは無視
  g_silent_reply = (am < 0 && !is_q);            // ブロードキャスト“設定系”は無応答にする

  /* ECHOは基本OFFだが、もしONならブロードキャスト時は抑止 */
  if (g_echo && !g_silent_reply) {
    CLI_Write("CMD=["); CLI_Write(&line[s]); CLI_Write("]\r\n");
  }

  /* ★この1行だけでOK（この後にもう1回呼んでる二重呼び出しを消す）*/
  handle_command(&line[s]);

  /* 次行の組み立てに備えてリセット */
  acc_len = 0;
}

/* ====== コマンド処理本体 =======================================
 * handle_command:
 * - cmd の内容に応じてデバイス制御・状態保存・UI 通知・応答送信を実施。
 * - 成功時は "OK\r\n"、エラー時は "ERR:..." を返す規約。
 * - 代表コマンド：
 *   * PING / *IDN?
 *   * MEAS:VOLT?                 ... ADC 計測（mV 表示）
 *   * SOUR:FREQ [Hz] / ?         ... DDS 周波数設定・問合せ
 *   * SOUR:FUNC SINE|TRI|SQU / ? ... DDS 波形設定・問合せ
 *   * SOUR:PHAS [deg] / ?        ... DDS 位相設定・問合せ（UI 通知付き）
 *   * SOUR:VOLT [V] / ?          ... DAC 全 CH 出力設定・問合せ
 *   * CLK INT|EXT                ... DDS MCLK の経路切替
 *   * ECHO ON|OFF                ... ECHO 出力の ON/OFF
 *   * POT:VOLT [V] / ?           ... DPOT 目標電圧（RUN 中は即反映 / UI 通知）
 *   * RUN / STOP / RUN?          ... 出力開始/停止/状態問合せ（UI 通知付き）
 * ===============================================================*/
static void handle_command(const char* cmd){
  if (!cmd || !*cmd) return;

  /* 汎用 */
  if (strcmp(cmd, "PING") == 0) { CLI_Write("PONG\r\n"); return; }
  if (strcmp(cmd, "*IDN?") == 0){ CLI_Write("KOS21,SignalGen,0.1.0\r\n"); return; }

  /* 測定: 電圧（例: CH4 / 16bit / ONESHOT / 1x）*/
  if (strcmp(cmd, "MEAS:VOLT?") == 0) {
    /* MCP3428 をワンショット 16bit で設定し、mV を取得 */
    MCP3428_SetConfig(&hadc3428,
                      MCP3428_CHANNEL_4,
                      MCP3428_RESOLUTION_16BIT,
                      MCP3428_MODE_ONESHOT,
                      MCP3428_GAIN_1X);
    int16_t mv = MCP3428_ReadMilliVolt(&hadc3428);
    if (mv == INT16_MIN) { write_err("ERR:MEAS"); }
    else {
      /* mV を "x.xxx\r\n" 形式に整形して応答 */
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
    /* DAC 全 CH を同一電圧に。apply_dac_all 内で範囲ガード＆丸め */
    if (apply_dac_all(v)) write_ok(); else write_err("ERR:DAC");
    return;
  }
  if (strcmp(cmd, "SOUR:VOLT?") == 0) {
    /* 現在の g_volt を "x.xxx\r\n" 形式に整形して応答 */
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

  /* デバッグ出力の切替（エコー ON/OFF） */
  if (strncmp(cmd, "ECHO ", 5) == 0) {
    const char* p = cmd + 5;
    if      (strncmp(p, "ON",  2) == 0) { g_echo = true;  write_ok(); }
    else if (strncmp(p, "OFF", 3) == 0) { g_echo = false; write_ok(); }
    else write_err("ERR:ARG");
    return;
  }

  /* AD5292: 目標電圧（RUN用）
   * - 値は保存。RUN 中なら DPOT に即反映（ライブ変更）。
   * - UI にも希望値として通知 (which=0)。
   */
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

  /* RUN / STOP / RUN?
   * - RUN/STOP では UI にも通知（画面の Run/Stop 表示を同期）
   * - STOP 時は AD5292_Set(0x400) で安全値へ（※設計方針）
   */
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
          apply_run_off();
          ui_notify_runstop(0);
      }
      write_ok(); return;
  }
  if (strcmp(cmd, "RUN?") == 0) {
    char buf[8]; snprintf(buf, sizeof(buf), "%u\r\n", (unsigned)g_run_state);
    CLI_Write(buf); return;
  }

  if (strcmp(cmd, "ADDR?") == 0) {
    char b[8]; snprintf(b, sizeof(b), "%X\r\n", (unsigned)g_addr);
    CLI_Write(b); return;
  }

  /* 未定義コマンド */
  write_err("ERR:UNKNOWN_CMD");
}


/* 先頭 "@<hex>" / "#<hex>" / "@*" を解釈する
 * 戻り値: 1=自分宛, 0=他人宛で無視, -1=ブロードキャストor宛先無し
 * 受理時は先頭の "@X[: ]*" を削って cmd をコマンド本体にする
 */
static int addr_match_and_strip(char* cmd){
  if (!cmd || !*cmd) return -1;
  if (cmd[0] == '@' || cmd[0] == '#'){
    if (cmd[1] == '*'){                   // broadcast
      char* p = cmd + 2;
      while (*p==' ' || *p==':' || *p=='-') p++;
      memmove(cmd, p, strlen(p)+1);
      return -1;
    }
    if (isxdigit((unsigned char)cmd[1]) && !isxdigit((unsigned char)cmd[2])) {
      uint8_t dst = (uint8_t)strtoul(&cmd[1], NULL, 16) & 0x0F;
      if (dst != g_addr) return 0;        // 他人宛 → 無視
      char* p = cmd + 2;
      while (*p==' ' || *p==':' || *p=='-') p++;
      memmove(cmd, p, strlen(p)+1);       // 自分宛 → プレフィクス除去
      return 1;
    }
  }
  return -1; // 宛先無し = ブロードキャスト扱い
}

/* “クエリか？”（'?' を含むか） */
static bool is_query_cmd(const char* s){
  return (s && strchr(s, '?') != NULL);
}

