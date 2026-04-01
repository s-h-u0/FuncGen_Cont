/*
===============================================================================
RS-485 Bridge (Controller Firmware) Notes / Protocol Memo
===============================================================================
目的
- PC(USART3) ⇔ Controller ⇔ RS-485 Bus(USART6) ⇔ Signal Generator
- PCからの1行コマンドを装置へ転送し、装置の応答行をPCへ中継する。
- UI（TouchGFX等）からも「1往復トランザクション」で安全に叩けるAPIを用意する
  (RS485_ENABLE_UI_API=1 の場合)。

UART / Pins
- PC_UART  : USART3 (huart3)
- BUS_UART : USART6 (huart6)  ※装置側CLIと接続
- RS-485方向制御: PB11 (L_Reci_H_Send)
	- 0 = RX mode
	- 1 = TX mode

受信設計（IRQ）
- HAL_UART_Receive_IT(..., 1byte) を常時回し、受信1byteごとにリングへ push。
- ISRでは重い処理をしない（push + 次のReceive_ITのみ）。

リングバッファ
- pc_rb  : PC→Controller 受信リング
- bus_rb : Device→Controller 受信リング
- rb_push(): 満杯なら捨てる（現状: “入れない”方式。tail進めて古いのを捨てる方式ではない）

送信（バス側）: bus_send_line()
- 送信時シーケンス（RS-485の核心）:
	1) RS485_TXmode()    // DE=1
	2) HAL_UART_Transmit(BUS_UART, line)
	3) HAL_UART_Transmit(BUS_UART, "\r\n")
	4) BUS_WaitTC()      // TC=Transmission Complete（stop bitまで出し切り）
	5) RS485_RXmode()    // DE=0
	6) delay_us(turnaround_us())  // ガード（切替直後のバタつき回避）
- turnaround_us() は概算1文字時間 + 余裕（現状: onechar_us + 500us）

ブリッジ動作: RS485_Bridge_Poll()
- 2フェーズ動作:
  A) PC→装置（コマンド送信）
	- pc_rb から1行を組み立て（CR/LFで確定）
	- 送信前に bus_rb の残骸を捨てる（前回応答の取り残し対策）
	- bus_send_line(cmd, log)
	- last_cmd に保存、waiting_reply=true、タイムアウト計測開始
  B) 装置→PC（応答集約）
	- bus_rb から受信を読み、'\n' で1行確定→ PCへ "<< " 付きで即出力
	- 無通信時間が閾値超え or 全体タイムアウトで終了判定
	- 完全無応答の場合のみ再送（MAX_RETRY, backoffあり）

UI向け 1往復 API（RS485_ENABLE_UI_API=1）
- RS485_Transact(...)
	- s_ui_busy による簡易排他（UI操作中はPC送信を保留する設計）
	- 送信前に bus_rb をクリア
	- bus_send_line(tx)
	- 受信は '\n' まで or timeout
	- ASCIIサニタイズ（任意）でUI表示を安全化

装置側（Signal Generator）の代表コマンド（ブリッジが流す前提）
- PING / *IDN?
- MEAS:VOLT?
- SOUR:FREQ <Hz> / SOUR:FREQ?
- SOUR:FUNC SINE|TRI|SQU / SOUR:FUNC?
- SOUR:PHAS <deg> / SOUR:PHAS?
- SOUR:VOLT <V> / SOUR:VOLT?
- CLK INT | CLK EXT
- POT:VOLT <V> / POT:VOLT?
- RUN / STOP / RUN?
- ADDR? / STATE?
- ECHO ON | ECHO OFF

ログ表記（PC側）
- 送信ログ: ">> <line>"
- 応答ログ: "<< <bytes...>"  ※改行含めて出す（pc_write_bin）
===============================================================================
*/


#ifndef RS485_BRIDGE_H
#define RS485_BRIDGE_H

#include "main.h"
#include "main.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RB_SZ               256
#define LINE_MAX            128
#define BUS_SILENCE_MS       80
#define RESP_TIMEOUT_MS    1000
#define MAX_RETRY             3
#define RETRY_BACKOFF_MS     50

#ifndef RS485_ENABLE_UI_API
#define RS485_ENABLE_UI_API 1
#endif

void RS485_Bridge_Init(void);
void RS485_Bridge_Poll(void);


// ===== UI行通知（イベント駆動）
typedef void (*rs485_ui_callback_t)(const char* line);
void RS485_RegisterUICallback(rs485_ui_callback_t cb);




// --- ここをこう分岐させる ---
#if RS485_ENABLE_UI_API

typedef enum {
    ORIGIN_PC = 0,
    ORIGIN_UI = 1
} rs485_origin_t;

bool RS485_IsBusy(void);

bool RS485_Transact(rs485_origin_t origin,
                    const char* line,
                    char* out, size_t out_sz,
                    uint32_t timeout_ms,
                    bool do_ascii_sanitize,
                    bool add_dip_prefix);

#else
// UI API 無効ビルド時：RS485_IsBusyは“常に空き”として使えるようにする
static inline bool RS485_IsBusy(void) { return false; }
#endif
#ifdef __cplusplus
}
#endif
#endif /* RS485_BRIDGE_H */
