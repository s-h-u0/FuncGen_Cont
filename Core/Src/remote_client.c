/*
 * remote_client.c
 *
 *  Created on: Sep 17, 2025
 *      Author: ssshu
 *
 * controller 側から RS-485 経由で target ノードの CLI を叩く薄いラッパ
 */

#include "remote_client.h"
#include "rs485_bridge.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static bool txrx_with_id(uint8_t id, const char* cmd, char* out, size_t out_sz, uint32_t to_ms);
static bool query_u32_to(uint8_t id, const char* cmd, uint32_t* out, uint32_t timeout_ms);
static bool query_i32_to(uint8_t id, const char* cmd, int32_t* out, uint32_t timeout_ms);
static bool query_bool_to(uint8_t id, const char* cmd, uint8_t* out, uint32_t timeout_ms);
static bool exec_expect_ok_to(uint8_t id, const char* fmt, long value, uint32_t timeout_ms);


/* ------------------------------------------------------------
 * 小ヘルパ
 * ------------------------------------------------------------ */

static bool parse_u32_reply(const char* s, uint32_t* out)
{
    char* endp = NULL;
    unsigned long v;

    if (!s || !out) return false;

    v = strtoul(s, &endp, 10);
    if (endp == s) return false;

    *out = (uint32_t)v;
    return true;
}

static bool parse_i32_reply(const char* s, int32_t* out)
{
    char* endp = NULL;
    long v;

    if (!s || !out) return false;

    v = strtol(s, &endp, 10);
    if (endp == s) return false;

    *out = (int32_t)v;
    return true;
}

/* "x.xxx" 形式を milli 単位へ */
static bool parse_fixed3_to_milli(const char* s, int32_t* out)
{
    long whole = 0;
    long frac  = 0;

    if (!s || !out) return false;

    if (sscanf(s, "%ld.%03ld", &whole, &frac) != 2) {
        return false;
    }

    *out = (int32_t)(whole * 1000 + ((whole >= 0) ? frac : -frac));
    return true;
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

bool remote_get_run_to(uint8_t id, uint8_t* run, uint32_t timeout_ms)
{
    return query_bool_to(id, "RUN?", run, timeout_ms);
}


bool remote_get_addr_to(uint8_t id, uint8_t* addr, uint32_t timeout_ms)
{
    char buf[16];
    uint32_t v = 0;

    if (!addr) return false;
    if (!txrx_with_id(id, "ADDR?", buf, sizeof(buf), timeout_ms)) return false;

    v = (uint32_t)strtoul(buf, NULL, 16) & 0x0FU;
    *addr = (uint8_t)v;
    return true;
}

bool remote_get_mclk_to(uint8_t id, uint8_t* on, uint32_t timeout_ms)
{
    return query_bool_to(id, "MCLK?", on, timeout_ms);
}


bool remote_get_sync_ready_to(uint8_t id, uint8_t* ready, uint32_t timeout_ms)
{
    return query_bool_to(id, "SYNC:READY?", ready, timeout_ms);
}


/* ------------------------------------------------------------
 * 出力設定
 * ------------------------------------------------------------ */


bool remote_set_freq_to(uint8_t id, uint32_t hz)
{
    return exec_expect_ok_to(id, "SOUR:FREQ %lu", (long)hz, 600);
}

bool remote_get_freq_to(uint8_t id, uint32_t* hz, uint32_t timeout_ms)
{
    return query_u32_to(id, "SOUR:FREQ?", hz, timeout_ms);
}

/*
bool remote_set_func_to(uint8_t id, const char* name)
{
    char cmd[48], b[8];
    snprintf(cmd, sizeof(cmd), "SOUR:FUNC %s", name);
    return txrx_with_id(id, cmd, b, sizeof(b), 600) && !strcmp(b, "OK");
}
*/

bool remote_set_phase_to(uint8_t id, uint16_t deg)
{
    if (deg > 359U) deg = 359U;
    return exec_expect_ok_to(id, "SOUR:PHAS %u", (long)deg, 800);
}


bool remote_get_phase_to(uint8_t id, uint32_t* deg, uint32_t timeout_ms)
{
    return query_u32_to(id, "SOUR:PHAS?", deg, timeout_ms);
}


bool remote_set_pot_volt_to(uint8_t id, uint32_t volt)
{
    if (volt > 40U) volt = 40U;
    return exec_expect_ok_to(id, "POT:VOLT %lu", (long)volt, 800);
}

bool remote_get_pot_volt_to(uint8_t id, uint32_t* volt, uint32_t timeout_ms)
{
    return query_u32_to(id, "POT:VOLT?", volt, timeout_ms);
}


/* ------------------------------------------------------------
 * トリップしきい値
 * ------------------------------------------------------------ */


bool remote_set_trip_curr_to(uint8_t id, uint32_t ma)
{
    return exec_expect_ok_to(id, "TRIP:CURR %lu", (long)ma, 600);
}

bool remote_get_trip_curr_to(uint8_t id, uint32_t* ma, uint32_t timeout_ms)
{
    return query_u32_to(id, "TRIP:CURR?", ma, timeout_ms);
}

bool remote_set_trip_volt_to(uint8_t id, uint32_t mv)
{
    return exec_expect_ok_to(id, "TRIP:VOLT %lu", (long)mv, 600);
}


bool remote_get_trip_volt_to(uint8_t id, uint32_t* mv, uint32_t timeout_ms)
{
    return query_u32_to(id, "TRIP:VOLT?", mv, timeout_ms);
}

bool remote_set_trip_temp_to(uint8_t id, int32_t deci_c)
{
    return exec_expect_ok_to(id, "TRIP:TEMP %ld", (long)deci_c, 600);
}

bool remote_get_trip_temp_to(uint8_t id, int32_t* deci_c, uint32_t timeout_ms)
{
    return query_i32_to(id, "TRIP:TEMP?", deci_c, timeout_ms);
}

/* ------------------------------------------------------------
 * 計測
 * ------------------------------------------------------------ */

bool remote_meas_volt_mV_to(uint8_t id, int32_t* mv, uint32_t to_ms)
{
    char buf[32];
    if (!mv) return false;
    if (!txrx_with_id(id, "MEAS:VOLT?", buf, sizeof(buf), to_ms)) {
        return false;
    }
    return parse_fixed3_to_milli(buf, mv);
}

bool remote_meas_curr_mA_to(uint8_t id, int32_t* ma, uint32_t to_ms)
{
    char buf[32];
    if (!ma) return false;
    if (!txrx_with_id(id, "MEAS:CURR?", buf, sizeof(buf), to_ms)) {
        return false;
    }
    return parse_fixed3_to_milli(buf, ma);
}

/* ------------------------------------------------------------
 * 状態通知要求
 * ------------------------------------------------------------ */

bool remote_request_state_report_to(uint8_t id, uint32_t timeout_ms)
{
    char b[8];
    /* target 側は複数行 STAT 通知 + 最後に OK */
    return txrx_with_id(id, "STATE?", b, sizeof(b), timeout_ms) &&
           (strcmp(b, "OK") == 0);
}


/* ------------------------------------------------------------
 * 同期制御
 * ------------------------------------------------------------ */

bool remote_sync_go_master(void)
{
    char b[8];
    return txrx_with_id(0x0U, "MCLK ON", b, sizeof(b), 600) && (strcmp(b, "OK") == 0);
}

bool remote_sync_stop_master(void)
{
    char b[8];
    return txrx_with_id(0x0U, "MCLK OFF", b, sizeof(b), 600) && (strcmp(b, "OK") == 0);
}

bool remote_sync_arm_all(void)
{
    /* broadcast 設定系は無応答 */
    return RS485_Transact(ORIGIN_UI,
                          "@* SYNC:ARM",
                          NULL, 0,
                          30,
                          true,
                          false);
}

bool remote_sync_release_all(void)
{
    /* broadcast 設定系は無応答 */
    return RS485_Transact(ORIGIN_UI,
                          "@* DDS:RELEASE",
                          NULL, 0,
                          30,
                          true,
                          false);
}

bool remote_query_sync_stat_to(uint8_t id, RemoteSyncStat* st, uint32_t timeout_ms)
{
    char buf[64];
    unsigned ready = 0, dirty = 0, mclk = 0;

    if (!st) return false;
    if (!txrx_with_id(id, "SYNC:STAT?", buf, sizeof(buf), timeout_ms)) return false;

    if (sscanf(buf, "READY=%u,DIRTY=%u,MCLK=%u", &ready, &dirty, &mclk) != 3) {
        return false;
    }

    st->ready = (uint8_t)(ready ? 1U : 0U);
    st->dirty = (uint8_t)(dirty ? 1U : 0U);
    st->mclk  = (uint8_t)(mclk  ? 1U : 0U);
    return true;
}

/* ------------------------------------------------------------
 * 内部実装
 * ------------------------------------------------------------ */

static bool txrx_with_id(uint8_t id, const char* cmd, char* out, size_t out_sz, uint32_t to_ms)
{
    char fullcmd[96];

    snprintf(fullcmd, sizeof(fullcmd), "@%X %s", (unsigned)(id & 0x0FU), cmd);

#ifdef REMOTE_CLIENT_DEBUG
    printf("TX(fullcmd): '%s'\r\n", fullcmd);
#endif

    return RS485_Transact(ORIGIN_UI, fullcmd, out, out_sz, to_ms, true, false);
}

static bool query_u32_to(uint8_t id, const char* cmd, uint32_t* out, uint32_t timeout_ms)
{
    char buf[24];
    if (!out) return false;
    if (!txrx_with_id(id, cmd, buf, sizeof(buf), timeout_ms)) return false;
    return parse_u32_reply(buf, out);
}

static bool query_i32_to(uint8_t id, const char* cmd, int32_t* out, uint32_t timeout_ms)
{
    char buf[24];
    if (!out) return false;
    if (!txrx_with_id(id, cmd, buf, sizeof(buf), timeout_ms)) return false;
    return parse_i32_reply(buf, out);
}

static bool query_bool_to(uint8_t id, const char* cmd, uint8_t* out, uint32_t timeout_ms)
{
    uint32_t v = 0;
    if (!out) return false;
    if (!query_u32_to(id, cmd, &v, timeout_ms)) return false;
    *out = (uint8_t)(v ? 1U : 0U);
    return true;
}

static bool exec_expect_ok_to(uint8_t id, const char* fmt, long value, uint32_t timeout_ms)
{
    char cmd[48];
    char b[8];

    snprintf(cmd, sizeof(cmd), fmt, value);
    return txrx_with_id(id, cmd, b, sizeof(b), timeout_ms) && (strcmp(b, "OK") == 0);
}
