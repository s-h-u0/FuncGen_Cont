#include "rs485_bridge.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

/* ====== どのUARTを使うか（Nucleo F412ZG 既定） ====== */
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;
#define PC_UART    huart3
#define BUS_UART   huart6

/* ====== RS485 DE/RE 制御ピン ====== */
#ifndef L_Reci_H_Send_Pin
# error "PB11(L_Reci_H_Send_Pin) が main.h に定義されていません"
#endif

/* ====== 内部状態（リングバッファ） ====== */
typedef struct { volatile uint8_t buf[RB_SZ]; volatile uint16_t head, tail; } rb_t;
static rb_t pc_rb  = { .head = 0, .tail = 0 };
static rb_t bus_rb = { .head = 0, .tail = 0 };
static volatile uint8_t pc_rx;   /* USART3 ISR が書く1バイト */
static volatile uint8_t bus_rx;  /* USART6 ISR が書く1バイト */

/* ====== ユーティリティ ====== */
static inline void RS485_RXmode(void){ HAL_GPIO_WritePin(L_Reci_H_Send_GPIO_Port, L_Reci_H_Send_Pin, GPIO_PIN_RESET); }
static inline void RS485_TXmode(void){ HAL_GPIO_WritePin(L_Reci_H_Send_GPIO_Port, L_Reci_H_Send_Pin, GPIO_PIN_SET); }
static inline void BUS_WaitTC(void){ while (__HAL_UART_GET_FLAG(&BUS_UART, UART_FLAG_TC) == RESET) { } }

static inline void rb_push(rb_t* r, uint8_t b){ uint16_t n = (uint16_t)((r->head + 1u) % RB_SZ); if (n != r->tail){ r->buf[r->head] = b; r->head = n; } }
static inline bool rb_pop (rb_t* r, uint8_t* out){ if (r->head == r->tail) return false; *out = r->buf[r->tail]; r->tail = (uint16_t)((r->tail + 1u) % RB_SZ); return true; }

/* 粗いusディレイ（SystemCoreClock依存） */
static void delay_us(uint32_t us){ uint32_t cycles = (SystemCoreClock / 1000000U) * us; while (cycles--) { __NOP(); } }

/* ボーレート依存ターンアラウンド（1文字時間 + 余裕） */
static inline uint32_t turnaround_us(void){
  uint32_t baud = BUS_UART.Init.BaudRate ? BUS_UART.Init.BaudRate : 115200U;
  uint32_t onechar_us = (11U * 1000000U) / baud; /* ≈1char */
  return onechar_us + 500U; /* ≈1.2msガード（環境により±500us調整） */
}

/* PCへログ出力 */
static void pc_write(const char* s){ if (!s || !*s) return; HAL_UART_Transmit(&PC_UART, (uint8_t*)s, (uint16_t)strlen(s), 200); }
static void pc_write_bin(const uint8_t* p, uint16_t n){ if (!p || !n) return; HAL_UART_Transmit(&PC_UART, (uint8_t*)p, n, 200); }
static void pc_log2(const char* tag, const char* s){ pc_write(tag); pc_write(s); pc_write("\r\n"); }

/* BUSへ行単位送信（CRLF付加, DE/RE制御：TX→TC→RX→guard） */
static void bus_send_line(const char* line){
  pc_log2(">> ", line);
  RS485_TXmode();
  size_t n = strlen(line);
  if (n) HAL_UART_Transmit(&BUS_UART, (uint8_t*)line, (uint16_t)n, 200);
  const uint8_t crlf[2] = { '\r', '\n' };
  HAL_UART_Transmit(&BUS_UART, (uint8_t*)crlf, 2, 200);
  BUS_WaitTC();
  RS485_RXmode();
  /* ★半二重の向き切替は“RXに戻した後”にガード時間をとる */
  delay_us(turnaround_us());
}

/* ====== 初期化 ====== */
void RS485_Bridge_Init(void){
  RS485_RXmode();
  HAL_UART_Receive_IT(&PC_UART,  (uint8_t*)&pc_rx,  1);
  HAL_UART_Receive_IT(&BUS_UART, (uint8_t*)&bus_rx, 1);

  pc_write("BOOT\r\n");

  /* 起動時に ECHO OFF を一度要求（応答待ちは通常ループに任せる） */
  bus_send_line("ECHO OFF");
  pc_log2("info ", "bridge ready");
}

/* ====== ポーリング ======
 * PC→装置 : 1行ずつ送信して“応答待ち”に。新コマンド送信前に装置側の取り残しを破棄。
 * 装置→PC : '\n'で行出力。無通信(BUS_SILENCE_MS)なら残りを吐き、完全無応答のみ [TIMEOUT]/再送。
 */
void RS485_Bridge_Poll(void){
  static char     cmd[LINE_MAX];
  static size_t   cmd_len = 0;

  /* ★ 送ったコマンドと再送カウント（既存） */
  static char     last_cmd[LINE_MAX];
  static uint8_t  retry_count = 0;

  static uint8_t  ans[RB_SZ];
  static size_t   ans_len = 0;
  static bool     got_any_reply = false;
  static uint32_t t_ref = 0;
  static bool     waiting_reply = false;

  uint8_t b;

  /* ==== PC→装置：行化して送信 ==== */
  if (!waiting_reply
#if RS485_ENABLE_UI_API
      && !RS485_IsBusy()        /* ★ UIの1往復中はPC送出を保留 */
#endif
     ){
    while (rb_pop(&pc_rb, &b)){
      if (b == '\r' || b == '\n'){
        if (cmd_len == 0) continue;                 /* 空行は捨てる */
        cmd[cmd_len] = '\0';

        /* 新コマンド送信前：装置→PCの取り残しを捨てる */
        uint8_t dump; while (rb_pop(&bus_rb, &dump)) {}

        /* コマンド送信 */
        bus_send_line(cmd);

        /* 送った内容を保持（無応答時の再送に使う） */
        strncpy(last_cmd, cmd, sizeof(last_cmd)-1);
        last_cmd[sizeof(last_cmd)-1] = '\0';
        retry_count = 0;

        cmd_len = 0;
        got_any_reply = false;
        waiting_reply = true;
        ans_len = 0;
        t_ref = HAL_GetTick();
        break;                                       /* 1行ずつ */
      }else{
        if (b == 0x08 || b == 0x7F){ if (cmd_len) cmd_len--; }
        else if (cmd_len + 1 < sizeof(cmd)){ cmd[cmd_len++] = (char)b; }
      }
    }
  }

  /* ==== 装置→PC：応答集約・吐き出し ==== */
  if (waiting_reply){
    while (rb_pop(&bus_rb, &b)){
      if (ans_len + 1 < sizeof(ans)) ans[ans_len++] = b;
      got_any_reply = true;
      if (b == '\n'){                                /* 行確定で即出力 */
        pc_write("<< ");
        pc_write_bin(ans, (uint16_t)ans_len);
        ans_len = 0;
      }
      t_ref = HAL_GetTick();
    }

    uint32_t dt = HAL_GetTick() - t_ref;

    /* 無通信しきい値 or 完全無応答のタイムアウト */
    if ((ans_len > 0 && dt >= BUS_SILENCE_MS) || (dt >= RESP_TIMEOUT_MS)) {
      if (ans_len > 0) {
        /* 行の途中で静かになった：溜まりを吐く */
        pc_write("<< ");
        pc_write_bin(ans, (uint16_t)ans_len);
        ans_len = 0;
        waiting_reply = false;                        /* これは“応答あり”扱い */
      } else {
        /* ★ 完全無応答：ここでだけ再送をかける */
        if (!got_any_reply && retry_count < MAX_RETRY){
          retry_count++;
          char note[32];
          snprintf(note, sizeof(note), "[RETRY %u/%u]", retry_count, (unsigned)MAX_RETRY);
          pc_log2("<< ", note);

          /* 取り残し破棄 → バックオフ → 再送 */
          uint8_t dump; while (rb_pop(&bus_rb, &dump)) {}
          HAL_Delay(RETRY_BACKOFF_MS);
          bus_send_line(last_cmd);

          /* 再度待ちへ */
          got_any_reply = false;
          ans_len = 0;
          t_ref = HAL_GetTick();
        } else {
          /* 再送上限を超えた or 途中で何かは来ていた → 失敗終了 */
          if (!got_any_reply) pc_log2("<< ", "[FAIL]");
          waiting_reply = false;
        }
      }
    }
  }
}

/* ====== UART 受信完了コールバック（ISR） ====== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* hu){
  if      (hu->Instance == USART3){ rb_push(&pc_rb,  (uint8_t)pc_rx);  HAL_UART_Receive_IT(&PC_UART,  (uint8_t*)&pc_rx,  1); }
  else if (hu->Instance == USART6){ rb_push(&bus_rb, (uint8_t)bus_rx); HAL_UART_Receive_IT(&BUS_UART, (uint8_t*)&bus_rx, 1); }
}

/* エラー後も受信を継続できるよう即再アーム（最小変更） */
void HAL_UART_ErrorCallback(UART_HandleTypeDef* hu){
  if      (hu->Instance == USART3){ HAL_UART_Receive_IT(&PC_UART,  (uint8_t*)&pc_rx,  1); }
  else if (hu->Instance == USART6){ HAL_UART_Receive_IT(&BUS_UART, (uint8_t*)&bus_rx, 1); }
}

/* =================================================================
 * ここから下：UI向け 1往復API（RS485_ENABLE_UI_API=1 のときだけ有効）
 * ================================================================= */
#if RS485_ENABLE_UI_API
#include "dipsw_221AMA16R.h"

static volatile bool s_ui_busy = false;
bool RS485_IsBusy(void){ return s_ui_busy; }

/* BUSリング残骸を捨てる（送信前にクリーンアップ） */
static void bus_drop_garbage(void){ uint8_t d; while (rb_pop(&bus_rb, &d)){} }

/* ASCIIサニタイズ（CR/LFのみ許可）。戻り値は新しい長さ */
static size_t ascii_sanitize(char* s, size_t len){
  size_t w=0;
  for (size_t i=0;i<len;++i){
    unsigned char c=(unsigned char)s[i];
    if (c=='\r' || c=='\n'){ s[w++]=(char)c; continue; }
    if (c>=0x20 && c<=0x7E){ s[w++]=(char)c; continue; }
  }
  s[w]='\0';
  return w;
}

bool RS485_Transact(rs485_origin_t origin,
                    const char* line,
                    char* out, size_t out_sz,
                    uint32_t timeout_ms,
                    bool do_ascii_sanitize,
                    bool add_dip_prefix)
{
	if (!line || !*line) return false;

	/* 占有開始（簡易スピン。UIスレッドからのみ呼ばれる想定） */
	 while (s_ui_busy) { /* wait */ }
	s_ui_busy = true;

	char tx[LINE_MAX + 8];
	if (add_dip_prefix) {
	    uint8_t dip = (uint8_t)(DIP221_Read() & 0x0F);
	    snprintf(tx, sizeof(tx), "@%X %s", (unsigned)dip, line);
	} else {
	    snprintf(tx, sizeof(tx), "%s", line);
	}

	/* 残骸クリア → 送信（TX→TC→RX→guard） */
	bus_drop_garbage();
	bus_send_line(tx);
  /* 受信：\n まで or timeout */
  size_t w = 0;
  uint32_t t0 = HAL_GetTick();
  if (out && out_sz) out[0] = '\0';

  for (;;){
    if ((HAL_GetTick() - t0) >= timeout_ms) break;

    uint8_t c;
    if (!rb_pop(&bus_rb, &c)) {
        HAL_Delay(1);   // 1msスリープ
        continue;
    }
    t0 = HAL_GetTick();                  /* 受信があればタイマ更新 */

    if (c == '\r') continue;
    if (c == '\n') break;
    if (do_ascii_sanitize && (c < 0x20 || c > 0x7E)) continue;
    if (out && w + 1 < out_sz) out[w++] = (char)c;
  }
  if (out && out_sz) out[w] = '\0';

  /* 任意のサニタイズ（改行は送っていないので不要だが保険で） */
  if (out && do_ascii_sanitize && w){
    (void)ascii_sanitize(out, w);
  }

  s_ui_busy = false;
  return (w > 0);
}
#endif /* RS485_ENABLE_UI_API */
