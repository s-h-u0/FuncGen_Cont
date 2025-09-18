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

static uint8_t g_currentID = 0;

static bool txrx_with_id(const char* cmd, char* out, size_t out_sz, uint32_t to);


/* 内部ユーティリティ: RS485でコマンド送信＆応答取得 */
static inline bool txrx(const char* cmd, char* out, size_t out_sz, uint32_t to) {
    return RS485_Transact(ORIGIN_UI, cmd, out, out_sz, to, true);
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
    return txrx(cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
}

bool remote_set_func(const char* name) {
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:FUNC %s", name);
    return txrx(cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
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
    if (!txrx("MEAS:VOLT?", buf, sizeof(buf), 800)) return false;

    long whole = 0, frac = 0;
    if (sscanf(buf, "%ld.%03ld", &whole, &frac) == 2) {
        if (mv) *mv = (int32_t)(whole * 1000 + (whole >= 0 ? frac : -frac));
        return true;
    }
    return false;
}

void remote_set_id(uint8_t id) {
    g_currentID = id & 0x0F;
}

// 共通ユーティリティ
static bool txrx_with_id(const char* cmd, char* out, size_t out_sz, uint32_t to) {
    char fullcmd[96];
    if (g_currentID)
        snprintf(fullcmd, sizeof(fullcmd), "@%X %s", g_currentID, cmd);
    else
        snprintf(fullcmd, sizeof(fullcmd), "%s", cmd);  // ID未設定ならプレフィクスなし
    return RS485_Transact(ORIGIN_UI, fullcmd, out, out_sz, to, true);
}
