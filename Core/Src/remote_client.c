/*
 * remote_client.c
 *
 *  Created on: Sep 17, 2025
 *      Author: ssshu
 */

#include "remote_client.h"
#include "rs485_bridge.h"
#include <stdio.h>
#include <string.h>


static bool txrx_with_id(const char* cmd, char* out, size_t out_sz, uint32_t to);


/* 内部ユーティリティ: RS485でコマンド送信＆応答取得 */
static inline bool txrx(const char* cmd, char* out, size_t out_sz, uint32_t to) {
    return RS485_Transact(ORIGIN_UI, cmd, out, out_sz, to, true, false);
}

/* --- 実装 --- */

bool remote_ping(char* out, size_t out_sz) {
    char buf[64];
    if (!txrx("PING", buf, sizeof(buf), 400)) return false;
    if (out && out_sz) {
        strncpy(out, buf, out_sz - 1);
        out[out_sz - 1] = 0;
    }
    return (strcmp(buf, "PONG") == 0);
}

bool remote_run(void) {
    char b[8];
    return txrx_with_id("RUN", b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_stop(void) {
    char b[8];
    return txrx_with_id("STOP", b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_set_freq(uint32_t hz) {
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:FREQ %lu", (unsigned long)hz);
    // 旧: return txrx(cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
    return txrx_with_id(cmd, b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_set_func(const char* name) {
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:FUNC %s", name);
    // 旧: return txrx(cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
    return txrx_with_id(cmd, b, sizeof(b), 600) && !strcmp(b, "OK");
}


bool remote_set_phas(uint16_t deg) {
    if (deg > 359) deg = 359;
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:PHAS %u", (unsigned)deg);
    return txrx_with_id(cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
}

bool remote_set_pot_volt(uint32_t volt) {
    if (volt > 40U) volt = 40U;
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "POT:VOLT %lu", (unsigned long)volt);
    return txrx_with_id(cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
}


bool remote_meas_volt_mV(int32_t* mv) {
    char buf[32];
    // 旧: if (!txrx("MEAS:VOLT?", buf, sizeof(buf), 800)) return false;
    if (!txrx_with_id("MEAS:VOLT?", buf, sizeof(buf), 200)) return false; // ← ID付き & 短めTO

    long whole = 0, frac = 0;
    if (sscanf(buf, "%ld.%03ld", &whole, &frac) == 2) {
        if (mv) *mv = (int32_t)(whole * 1000 + (whole >= 0 ? frac : -frac));
        return true;
    }
    return false;
}


bool remote_meas_volt_mV_to(int32_t* mv, uint32_t to_ms) {
    char cmd[16], buf[32];
    snprintf(cmd, sizeof(cmd), "@%X MEAS:VOLT?", g_currentID & 0x0F);
    if (!RS485_Transact(ORIGIN_UI, cmd, buf, sizeof(buf), to_ms, true, false))
        return false;

    long whole = 0, frac = 0;
    if (sscanf(buf, "%ld.%03ld", &whole, &frac) == 2) {
        if (mv) *mv = (int32_t)(whole * 1000 + (whole >= 0 ? frac : -frac));
        return true;
    }
    return false;
}



// --- UI側(MainPresenterなど)で管理するg_currentIDを参照する ---
// グローバル定義を削除して、外部参照にする
extern uint8_t g_currentID;

void remote_set_id(uint8_t id) {
    g_currentID = id & 0x0F;  // UIの実体を更新
}

uint8_t remote_get_id(void) {
    return g_currentID;       // UIの実体を返す
}


static bool txrx_with_id(const char* cmd, char* out, size_t out_sz, uint32_t to)
{
    char fullcmd[96];
    // ★ 常に @ID を付けて送信する（0でも）
    snprintf(fullcmd, sizeof(fullcmd), "@%X %s", g_currentID, cmd);

    // ★ デバッグ出力
    printf("TX(fullcmd): '%s'\r\n", fullcmd);

    return RS485_Transact(ORIGIN_UI, fullcmd, out, out_sz, to, false, false);
}

bool remote_query_state(void){
    char b[8];
    // 旧: RS485_Transact(..., "STATE?", ...)
    return txrx_with_id("STATE?", b, sizeof(b), 200);
}

