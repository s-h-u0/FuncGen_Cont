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
#include <stdlib.h>
#include <string.h>

#define REMOTE_ID_MASK                  0x0FU
#define REMOTE_REPLY_OK_SIZE            8U
#define REMOTE_REPLY_VALUE_SIZE         24U
#define REMOTE_MEAS_REPLY_SIZE          32U
#define REMOTE_CMD_SIZE                 48U
#define REMOTE_FULLCMD_SIZE             96U

#define REMOTE_TIMEOUT_BROADCAST_MS     30U
#define REMOTE_TIMEOUT_PING_MS          400U
#define REMOTE_TIMEOUT_DEFAULT_MS       600U
#define REMOTE_TIMEOUT_SETTING_MS       800U
#define REMOTE_TIMEOUT_PRIME_MS         1000U

static bool txrx_with_id(uint8_t id,
                         const char* cmd,
                         char* out,
                         size_t out_sz,
                         uint32_t timeout_ms);

static bool exec_cmd_expect_ok_to(uint8_t id,
                                  const char* cmd,
                                  uint32_t timeout_ms);
static bool exec_expect_ok_to(uint8_t id,
                              const char* fmt,
                              long value,
                              uint32_t timeout_ms);

static bool query_u32_to(uint8_t id,
                         const char* cmd,
                         uint32_t* out,
                         uint32_t timeout_ms);
static bool query_i32_to(uint8_t id,
                         const char* cmd,
                         int32_t* out,
                         uint32_t timeout_ms);
static bool query_bool_to(uint8_t id,
                          const char* cmd,
                          uint8_t* out,
                          uint32_t timeout_ms);

static bool reply_is_ok(const char* s);
static bool reply_is_pong(const char* s);
static bool parse_u32_reply(const char* s, uint32_t* out);
static bool parse_i32_reply(const char* s, int32_t* out);
static bool parse_fixed3_to_milli(const char* s, int32_t* out);

/* ------------------------------------------------------------
 * 返信判定 / 数値変換
 * ------------------------------------------------------------ */

static bool reply_is_ok(const char* s)
{
    return (s && strcmp(s, "OK") == 0);
}

static bool reply_is_pong(const char* s)
{
    return (s && strcmp(s, "PONG") == 0);
}

static bool parse_u32_reply(const char* s, uint32_t* out)
{
    char* endp = NULL;
    unsigned long v;

    if (!s || !out) {
        return false;
    }

    v = strtoul(s, &endp, 10);
    if (endp == s) {
        return false;
    }

    *out = (uint32_t)v;
    return true;
}

static bool parse_i32_reply(const char* s, int32_t* out)
{
    char* endp = NULL;
    long v;

    if (!s || !out) {
        return false;
    }

    v = strtol(s, &endp, 10);
    if (endp == s) {
        return false;
    }

    *out = (int32_t)v;
    return true;
}

/* "x.xxx" 形式を milli 単位へ変換する。 */
static bool parse_fixed3_to_milli(const char* s, int32_t* out)
{
    long whole = 0;
    long frac  = 0;

    if (!s || !out) {
        return false;
    }

    if (sscanf(s, "%ld.%03ld", &whole, &frac) != 2) {
        return false;
    }

    *out = (int32_t)(whole * 1000 + ((whole >= 0) ? frac : -frac));
    return true;
}

/* ------------------------------------------------------------
 * 内部通信ヘルパ
 * ------------------------------------------------------------ */

static bool txrx_with_id(uint8_t id,
                         const char* cmd,
                         char* out,
                         size_t out_sz,
                         uint32_t timeout_ms)
{
    char fullcmd[REMOTE_FULLCMD_SIZE];

    snprintf(fullcmd, sizeof(fullcmd), "@%X %s", (unsigned)(id & REMOTE_ID_MASK), cmd);

#ifdef REMOTE_CLIENT_DEBUG
    printf("TX(fullcmd): '%s'\r\n", fullcmd);
#endif

    return RS485_Transact(ORIGIN_UI, fullcmd, out, out_sz, timeout_ms, true, false);
}

static bool exec_cmd_expect_ok_to(uint8_t id,
                                  const char* cmd,
                                  uint32_t timeout_ms)
{
    char reply[REMOTE_REPLY_OK_SIZE];

    return txrx_with_id(id, cmd, reply, sizeof(reply), timeout_ms) &&
           reply_is_ok(reply);
}

static bool exec_expect_ok_to(uint8_t id,
                              const char* fmt,
                              long value,
                              uint32_t timeout_ms)
{
    char cmd[REMOTE_CMD_SIZE];

    snprintf(cmd, sizeof(cmd), fmt, value);
    return exec_cmd_expect_ok_to(id, cmd, timeout_ms);
}

static bool query_u32_to(uint8_t id,
                         const char* cmd,
                         uint32_t* out,
                         uint32_t timeout_ms)
{
    char buf[REMOTE_REPLY_VALUE_SIZE];

    if (!out) {
        return false;
    }
    if (!txrx_with_id(id, cmd, buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    return parse_u32_reply(buf, out);
}

static bool query_i32_to(uint8_t id,
                         const char* cmd,
                         int32_t* out,
                         uint32_t timeout_ms)
{
    char buf[REMOTE_REPLY_VALUE_SIZE];

    if (!out) {
        return false;
    }
    if (!txrx_with_id(id, cmd, buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    return parse_i32_reply(buf, out);
}

static bool query_bool_to(uint8_t id,
                          const char* cmd,
                          uint8_t* out,
                          uint32_t timeout_ms)
{
    uint32_t v = 0;

    if (!out) {
        return false;
    }
    if (!query_u32_to(id, cmd, &v, timeout_ms)) {
        return false;
    }

    *out = (uint8_t)(v ? 1U : 0U);
    return true;
}

/* ------------------------------------------------------------
 * 基本制御
 * ------------------------------------------------------------ */

bool remote_ping_to_timeout(uint8_t id,
                            char* out,
                            size_t out_sz,
                            uint32_t timeout_ms)
{
    char buf[64];

    if (!txrx_with_id(id, "PING", buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    if (out && out_sz) {
        strncpy(out, buf, out_sz - 1U);
        out[out_sz - 1U] = '\0';
    }

    return reply_is_pong(buf);
}

bool remote_ping_to(uint8_t id, char* out, size_t out_sz)
{
    return remote_ping_to_timeout(id, out, out_sz, REMOTE_TIMEOUT_PING_MS);
}

bool remote_run_to(uint8_t id)
{
    return exec_cmd_expect_ok_to(id, "RUN", REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_stop_to(uint8_t id)
{
    return exec_cmd_expect_ok_to(id, "STOP", REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_get_run_to(uint8_t id, uint8_t* run, uint32_t timeout_ms)
{
    return query_bool_to(id, "RUN?", run, timeout_ms);
}

/* ------------------------------------------------------------
 * 出力設定
 * ------------------------------------------------------------ */

bool remote_set_freq_to(uint8_t id, uint32_t hz)
{
    return exec_expect_ok_to(id, "SOUR:FREQ %lu", (long)hz, REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_get_freq_to(uint8_t id, uint32_t* hz, uint32_t timeout_ms)
{
    return query_u32_to(id, "SOUR:FREQ?", hz, timeout_ms);
}

bool remote_set_phase_to(uint8_t id, uint16_t deg)
{
    if (deg > 359U) {
        deg = 359U;
    }

    return exec_expect_ok_to(id, "SOUR:PHAS %u", (long)deg, REMOTE_TIMEOUT_SETTING_MS);
}

bool remote_get_phase_to(uint8_t id, uint32_t* deg, uint32_t timeout_ms)
{
    return query_u32_to(id, "SOUR:PHAS?", deg, timeout_ms);
}

bool remote_set_pot_volt_to(uint8_t id, uint32_t volt)
{
    if (volt > 40U) {
        volt = 40U;
    }

    return exec_expect_ok_to(id, "POT:VOLT %lu", (long)volt, REMOTE_TIMEOUT_SETTING_MS);
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
    return exec_expect_ok_to(id, "TRIP:CURR %lu", (long)ma, REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_get_trip_curr_to(uint8_t id, uint32_t* ma, uint32_t timeout_ms)
{
    return query_u32_to(id, "TRIP:CURR?", ma, timeout_ms);
}

bool remote_set_trip_volt_to(uint8_t id, uint32_t mv)
{
    return exec_expect_ok_to(id, "TRIP:VOLT %lu", (long)mv, REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_get_trip_volt_to(uint8_t id, uint32_t* mv, uint32_t timeout_ms)
{
    return query_u32_to(id, "TRIP:VOLT?", mv, timeout_ms);
}

bool remote_set_trip_temp_to(uint8_t id, int32_t deci_c)
{
    return exec_expect_ok_to(id, "TRIP:TEMP %ld", (long)deci_c, REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_get_trip_temp_to(uint8_t id, int32_t* deci_c, uint32_t timeout_ms)
{
    return query_i32_to(id, "TRIP:TEMP?", deci_c, timeout_ms);
}

/* ------------------------------------------------------------
 * 計測
 * ------------------------------------------------------------ */

bool remote_meas_volt_mV_to(uint8_t id, int32_t* mv, uint32_t timeout_ms)
{
    char buf[REMOTE_MEAS_REPLY_SIZE];

    if (!mv) {
        return false;
    }
    if (!txrx_with_id(id, "MEAS:VOLT?", buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    return parse_fixed3_to_milli(buf, mv);
}

bool remote_meas_curr_mA_to(uint8_t id, int32_t* ma, uint32_t timeout_ms)
{
    char buf[REMOTE_MEAS_REPLY_SIZE];

    if (!ma) {
        return false;
    }
    if (!txrx_with_id(id, "MEAS:CURR?", buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    return parse_fixed3_to_milli(buf, ma);
}

/* ------------------------------------------------------------
 * 状態取得
 * ------------------------------------------------------------ */

bool remote_request_state_report_to(uint8_t id, uint32_t timeout_ms)
{
    /* target 側は複数行 STAT 通知 + 最後に OK */
    return exec_cmd_expect_ok_to(id, "STATE?", timeout_ms);
}

bool remote_get_addr_to(uint8_t id, uint8_t* addr, uint32_t timeout_ms)
{
    char buf[16];
    uint32_t v = 0;

    if (!addr) {
        return false;
    }
    if (!txrx_with_id(id, "ADDR?", buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    v = (uint32_t)strtoul(buf, NULL, 16) & REMOTE_ID_MASK;
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

bool remote_query_sync_stat_to(uint8_t id, RemoteSyncStat* st, uint32_t timeout_ms)
{
    char buf[64];
    unsigned ready = 0;
    unsigned dirty = 0;
    unsigned mclk  = 0;

    if (!st) {
        return false;
    }
    if (!txrx_with_id(id, "SYNC:STAT?", buf, sizeof(buf), timeout_ms)) {
        return false;
    }

    if (sscanf(buf, "READY=%u,DIRTY=%u,MCLK=%u", &ready, &dirty, &mclk) != 3) {
        return false;
    }

    st->ready = (uint8_t)(ready ? 1U : 0U);
    st->dirty = (uint8_t)(dirty ? 1U : 0U);
    st->mclk  = (uint8_t)(mclk  ? 1U : 0U);
    return true;
}

/* ------------------------------------------------------------
 * 同期制御
 * ------------------------------------------------------------ */

bool remote_sync_go_master(void)
{
    return exec_cmd_expect_ok_to(0x0U, "MCLK ON", REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_sync_stop_master(void)
{
    return exec_cmd_expect_ok_to(0x0U, "MCLK OFF", REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_sync_prime_master(void)
{
    return exec_cmd_expect_ok_to(0x0U, "MCLK PRIME", REMOTE_TIMEOUT_PRIME_MS);
}

bool remote_sync_arm_all(void)
{
    /* broadcast 設定系は無応答 */
    return RS485_Transact(ORIGIN_UI,
                          "@* SYNC:ARM",
                          NULL,
                          0,
                          REMOTE_TIMEOUT_BROADCAST_MS,
                          true,
                          false);
}

bool remote_sync_release_all(void)
{
    /* broadcast 設定系は無応答 */
    return RS485_Transact(ORIGIN_UI,
                          "@* DDS:RELEASE",
                          NULL,
                          0,
                          REMOTE_TIMEOUT_BROADCAST_MS,
                          true,
                          false);
}

bool remote_sync_arm_to(uint8_t id)
{
    return exec_cmd_expect_ok_to(id, "SYNC:ARM", REMOTE_TIMEOUT_DEFAULT_MS);
}

bool remote_sync_release_to(uint8_t id)
{
    return exec_cmd_expect_ok_to(id, "DDS:RELEASE", REMOTE_TIMEOUT_DEFAULT_MS);
}
