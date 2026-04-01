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



static bool txrx_with_id(uint8_t id, const char* cmd, char* out, size_t out_sz, uint32_t to);

/* 内部ユーティリティ: RS485でコマンド送信＆応答取得 */
static inline bool txrx(const char* cmd, char* out, size_t out_sz, uint32_t to)
{
    return RS485_Transact(ORIGIN_UI, cmd, out, out_sz, to, true, false);
}


/* ===== 新API: 明示的にIDを渡す ===== */

bool remote_ping_to(uint8_t id, char* out, size_t out_sz)
{
    char buf[64];
    if (!txrx_with_id(id, "PING", buf, sizeof(buf), 400)) {
        return false;
    }

    if (out && out_sz) {
        strncpy(out, buf, out_sz - 1);
        out[out_sz - 1] = 0;
    }
    return (strcmp(buf, "PONG") == 0);
}

bool remote_run_to(uint8_t id)
{
    char b[8];
    return txrx_with_id(id, "RUN", b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_stop_to(uint8_t id)
{
    char b[8];
    return txrx_with_id(id, "STOP", b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_set_freq_to(uint8_t id, uint32_t hz)
{
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:FREQ %lu", (unsigned long)hz);
    return txrx_with_id(id, cmd, b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_set_func_to(uint8_t id, const char* name)
{
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:FUNC %s", name);
    return txrx_with_id(id, cmd, b, sizeof(b), 600) && !strcmp(b, "OK");
}

bool remote_set_phas_to(uint8_t id, uint16_t deg)
{
    if (deg > 359) deg = 359;
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:PHAS %u", (unsigned)deg);
    return txrx_with_id(id, cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
}

bool remote_set_pot_volt_to(uint8_t id, uint32_t volt)
{
    if (volt > 40U) volt = 40U;
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "POT:VOLT %lu", (unsigned long)volt);
    return txrx_with_id(id, cmd, b, sizeof(b), 800) && !strcmp(b, "OK");
}

bool remote_meas_volt_mV_to(uint8_t id, int32_t* mv, uint32_t to_ms)
{
    char buf[32];
    if (!txrx_with_id(id, "MEAS:VOLT?", buf, sizeof(buf), to_ms)) {
        return false;
    }

    long whole = 0, frac = 0;
    if (sscanf(buf, "%ld.%03ld", &whole, &frac) == 2) {
        if (mv) {
            *mv = (int32_t)(whole * 1000 + (whole >= 0 ? frac : -frac));
        }
        return true;
    }
    return false;
}

bool remote_meas_curr_mV_to(uint8_t id, int32_t* mv, uint32_t to_ms)
{
    char buf[32];
    if (!txrx_with_id(id, "MEAS:CURR?", buf, sizeof(buf), to_ms)) {
        return false;
    }

    long whole = 0, frac = 0;
    if (sscanf(buf, "%ld.%03ld", &whole, &frac) == 2) {
        if (mv) {
            *mv = (int32_t)(whole * 1000 + (whole >= 0 ? frac : -frac));
        }
        return true;
    }
    return false;
}

bool remote_query_state_to(uint8_t id)
{
    char b[8];
    return txrx_with_id(id, "STATE?", b, sizeof(b), 200);
}

/* ===== 内部実装 ===== */

static bool txrx_with_id(uint8_t id, const char* cmd, char* out, size_t out_sz, uint32_t to)
{
    char fullcmd[96];
    snprintf(fullcmd, sizeof(fullcmd), "@%X %s", (unsigned)(id & 0x0F), cmd);

#ifdef REMOTE_CLIENT_DEBUG
    printf("TX(fullcmd): '%s'\r\n", fullcmd);
#endif

    return RS485_Transact(ORIGIN_UI, fullcmd, out, out_sz, to, true, false);
}
