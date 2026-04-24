/*
 * app_remote.c
 *
 *  Created on: Mar 31, 2026
 *      Author: shu-morishima
 */


#include "app_remote.h"
#include "remote_client.h"
#include "rs485_bridge.h"
#include "main.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef APPREMOTE_LINE_MAX
#define APPREMOTE_LINE_MAX 64
#endif

#ifndef APPREMOTE_Q_SZ
#define APPREMOTE_Q_SZ 32
#endif

typedef struct {
    uint8_t current_id;
} app_remote_state_t;

static app_remote_state_t s;

/* ===== 受信キュー ===== */
static volatile uint8_t s_q_head = 0;
static volatile uint8_t s_q_tail = 0;
static char s_q_lines[APPREMOTE_Q_SZ][APPREMOTE_LINE_MAX];

#ifndef APPREMOTE_SYNC_FIRST_ID
#define APPREMOTE_SYNC_FIRST_ID 0U
#endif

#ifndef APPREMOTE_SYNC_LAST_ID
#define APPREMOTE_SYNC_LAST_ID 7U
#endif

#ifndef APPREMOTE_SYNC_QUERY_TIMEOUT_MS
#define APPREMOTE_SYNC_QUERY_TIMEOUT_MS 200U
#endif

static bool query_sync_group(uint8_t begin,
                             uint8_t end,
                             uint8_t expect_ready,
                             uint8_t require_clean,
                             uint32_t to_ms);



static void appremote_queue_push(const char* line)
{
    if (!line || !*line) return;

    size_t n = strnlen(line, APPREMOTE_LINE_MAX - 1);

    __disable_irq();
    uint8_t next = (uint8_t)((s_q_head + 1U) % APPREMOTE_Q_SZ);

    /* フル時は最古を捨てる */
    if (next == s_q_tail) {
        s_q_tail = (uint8_t)((s_q_tail + 1U) % APPREMOTE_Q_SZ);
    }

    memcpy(s_q_lines[s_q_head], line, n);
    s_q_lines[s_q_head][n] = '\0';
    s_q_head = next;
    __enable_irq();
}

static bool appremote_queue_pop(char* out, size_t out_sz)
{
    if (!out || out_sz == 0U) return false;

    __disable_irq();
    if (s_q_tail == s_q_head) {
        __enable_irq();
        return false;
    }

    strncpy(out, s_q_lines[s_q_tail], out_sz - 1U);
    out[out_sz - 1U] = '\0';
    s_q_tail = (uint8_t)((s_q_tail + 1U) % APPREMOTE_Q_SZ);
    __enable_irq();

    return true;
}

/* RS-485 UI callback trampoline */
static void AppRemote_RS485Callback(const char* line)
{
    appremote_queue_push(line);
}

void AppRemote_Init(void)
{
    s.current_id = 0;

    s_q_head = 0;
    s_q_tail = 0;

    RS485_Bridge_Init();
    RS485_RegisterUICallback(AppRemote_RS485Callback);
}

void AppRemote_SetID(uint8_t id)
{
    s.current_id = (uint8_t)(id & 0x0F);
}

uint8_t AppRemote_GetID(void)
{
    return s.current_id;
}


/* AppRemote は通信ユースケース層であり、RUN/STOP 状態は保持しない。
 * ここでは current_id 宛てにコマンドを送るだけで、
 * 状態の確定反映は MainPresenter::onRemoteLine() 側で行う。
 */
bool AppRemote_Run(void)
{
    if (remote_run_to(s.current_id)) {
        return true;
    }
    return false;
}

bool AppRemote_Stop(void)
{
    if (remote_stop_to(s.current_id)) {
        return true;
    }
    return false;
}

bool AppRemote_QueryState(void)
{
    return remote_request_state_report_to(s.current_id, 600);
}

bool AppRemote_MeasVolt(int32_t* mv, uint32_t to_ms)
{
    return remote_meas_volt_mV_to(s.current_id, mv, to_ms);
}

bool AppRemote_MeasCurr(int32_t* ma, uint32_t to_ms)
{
    return remote_meas_curr_mA_to(s.current_id, ma, to_ms);
}

bool AppRemote_SetVolt(uint32_t mv)
{
    return remote_set_pot_volt_to(s.current_id, mv);
}

bool AppRemote_SetTripCurr(uint32_t ma)
{
    return remote_set_trip_curr_to(s.current_id, ma);
}

bool AppRemote_SetCurr(uint32_t ma)
{
    return AppRemote_SetTripCurr(ma);
}

bool AppRemote_SetPhas(uint16_t deg)
{
    return remote_set_phase_to(s.current_id, deg);
}


void AppRemote_HandleLine(const char* line)
{
    appremote_queue_push(line);
}

bool AppRemote_PopLine(char* out, size_t out_sz)
{
    return appremote_queue_pop(out, out_sz);
}

bool AppRemote_ParseLine(const char* line, AppRemote_Event* ev)
{
    if (!line || !ev) return false;

    char buf[64];
    size_t len = strlen(line);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1U;

    for (size_t i = 0; i < len; ++i) {
        buf[i] = (char)tolower((unsigned char)line[i]);
    }
    buf[len] = '\0';

    ev->type = APPREMOTE_EVT_NONE;
    ev->id = 0xFF;
    ev->value = 0;

    const char* p = buf;
    if ((p[0] == '#' || p[0] == '@') && isxdigit((unsigned char)p[1])) {
        ev->id = (uint8_t)strtoul(&p[1], NULL, 16) & 0x0F;
        p += 2;
        while (*p == ' ' || *p == ':') ++p;
    }

    if (strcmp(p, "run") == 0) {
        ev->type = APPREMOTE_EVT_RUN;
        return true;
    }

    if (strcmp(p, "stop") == 0) {
        ev->type = APPREMOTE_EVT_STOP;
        return true;
    }

    if (strncmp(p, "stat:volt", 9) == 0) {
        p += 9;
        while (*p == ' ' || *p == ':') ++p;
        ev->type = APPREMOTE_EVT_STAT_VOLT;
        ev->value = (uint32_t)(atoi(p) / 1000);
        return true;
    }

    if (strncmp(p, "stat:phas", 9) == 0) {
        p += 9;
        while (*p == ' ' || *p == ':') ++p;
        ev->type = APPREMOTE_EVT_STAT_PHAS;
        ev->value = (uint32_t)atoi(p);
        return true;
    }

    if (strncmp(p, "stat:tcurr", 10) == 0) {
        p += 10;
        while (*p == ' ' || *p == ':') ++p;
        ev->type = APPREMOTE_EVT_STAT_TRIP_CURR;
        ev->value = (uint32_t)atoi(p);
        return true;
    }

    if (strncmp(p, "stat:freq", 9) == 0) {
        p += 9;
        while (*p == ' ' || *p == ':') ++p;
        ev->type = APPREMOTE_EVT_STAT_FREQ;
        ev->value = (uint32_t)strtoul(p, NULL, 10);
        return true;
    }

    if (strncmp(p, "stat:ready", 10) == 0) {
        p += 10;
        while (*p == ' ' || *p == ':') ++p;
        ev->type = APPREMOTE_EVT_STAT_READY;
        ev->value = (uint32_t)strtoul(p, NULL, 10);
        return true;
    }

    if (strncmp(p, "stat:mclk", 9) == 0) {
        p += 9;
        while (*p == ' ' || *p == ':') ++p;
        ev->type = APPREMOTE_EVT_STAT_MCLK;
        ev->value = (uint32_t)strtoul(p, NULL, 10);
        return true;
    }

    return false;
}


void AppRemote_Process(void)
{
    RS485_Bridge_Poll();
}

bool AppRemote_SyncGo(void)
{
    return remote_sync_go_master();
}

static uint8_t s_last_sync_ok[8] = {0};

static void clear_last_sync_ok(void)
{
    for (uint8_t i = 0; i < 8; ++i) {
        s_last_sync_ok[i] = 0;
    }
}

static bool verify_sync_group(uint8_t begin, uint8_t end)
{
    bool all_ok = true;

    for (uint8_t id = begin; id <= end; ++id) {
        RemoteSyncStat st;

        if (remote_query_sync_stat_to(id, &st, 200) &&
            st.ready == 1 &&
            st.dirty == 0) {
            s_last_sync_ok[id] = 1;
        } else {
            s_last_sync_ok[id] = 0;
            all_ok = false;
        }
    }

    return all_ok;
}

static bool query_sync_group(uint8_t begin,
                             uint8_t end,
                             uint8_t expect_ready,
                             uint8_t require_clean,
                             uint32_t to_ms)
{
    bool all_ok = true;

    if (end >= 8U) {
        end = 7U;
    }

    for (uint8_t id = begin; id <= end; ++id) {
        RemoteSyncStat st;

        if (!remote_query_sync_stat_to(id, &st, to_ms)) {
            s_last_sync_ok[id] = 0U;
            all_ok = false;
            continue;
        }

        if (st.ready != expect_ready) {
            s_last_sync_ok[id] = 0U;
            all_ok = false;
            continue;
        }

        if (require_clean && st.dirty != 0U) {
            s_last_sync_ok[id] = 0U;
            all_ok = false;
            continue;
        }

        s_last_sync_ok[id] = 1U;
    }

    return all_ok;
}



bool AppRemote_GetLastSyncOk(uint8_t id)
{
    if (id >= 8) return false;
    return s_last_sync_ok[id] != 0;
}



bool AppRemote_SyncStart(void)
{
    clear_last_sync_ok();

    /* 1) 全ノードへ ARM
     * broadcast 無応答なので、戻り値では判定しない
     */
    remote_sync_arm_all();
    HAL_Delay(100);

    /* 2) ARM後、まだ RELEASE 前なので READY=0 を確認
     *
     */
    if (!query_sync_group(APPREMOTE_SYNC_FIRST_ID,
                          APPREMOTE_SYNC_LAST_ID,
                          0U,
                          0U,
                          APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
        return false;
    }

    /* 3) master(node0) の共有MCLK配布を停止 */
    if (!AppRemote_SyncStopMaster()) return false;
    HAL_Delay(20);

    /* 4) 全ノードで DDS reset 解除
     * これも broadcast 無応答なので、戻り値では判定しない
     */
    remote_sync_release_all();
    HAL_Delay(100);

    /* 5) RELEASE後、0〜7 全ノードが READY=1 かつ DIRTY=0 か確認 */
    if (!query_sync_group(APPREMOTE_SYNC_FIRST_ID,
                          APPREMOTE_SYNC_LAST_ID,
                          1U,
                          1U,
                          APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
        return false;
    }

    /* 6) master(node0) で共有MCLK配布開始 */
    if (!AppRemote_SyncGoMaster()) return false;
    HAL_Delay(50);

    /* 7) 最終的に 0〜7ch を順に確認し、結果を保存 */
    return verify_sync_group(APPREMOTE_SYNC_FIRST_ID,
                             APPREMOTE_SYNC_LAST_ID);
}

bool AppRemote_SyncArmAll(void)
{
    return remote_sync_arm_all();
}

bool AppRemote_SyncGoMaster(void)
{
    return remote_sync_go_master();
}

bool AppRemote_SyncStopMaster(void)
{
    return remote_sync_stop_master();
}

bool AppRemote_SyncReleaseAll(void)
{
    return remote_sync_release_all();
}

bool AppRemote_QuerySyncStat(RemoteSyncStat* st, uint32_t to_ms)
{
    return remote_query_sync_stat_to(s.current_id, st, to_ms);
}

