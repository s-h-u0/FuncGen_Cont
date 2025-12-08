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
#define RS485_ENABLE_UI_API 0
#endif

void RS485_Bridge_Init(void);
void RS485_Bridge_Poll(void);


// ===== UI行通知（イベント駆動）
typedef void (*rs485_ui_callback_t)(const char* line);
void RS485_RegisterUICallback(rs485_ui_callback_t cb);

bool RS485_PcHasPending(void);   // PC側(UART3)に未送出の入力があるか


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
