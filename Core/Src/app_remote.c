/*
 * app_remote.c
 *
 *  Created on: Mar 31, 2026
 *      Author: shu-morishima
 */

#include "app_remote.h"

#include "main.h"
#include "remote_client.h"
#include "rs485_bridge.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifndef APPREMOTE_LINE_MAX
#define APPREMOTE_LINE_MAX 64U
#endif

#ifndef APPREMOTE_Q_SZ
#define APPREMOTE_Q_SZ 32U
#endif

#ifndef APPREMOTE_NODE_MAX
#define APPREMOTE_NODE_MAX 8U
#endif

#ifndef APPREMOTE_SYNC_FIRST_ID
#define APPREMOTE_SYNC_FIRST_ID 0U
#endif

#ifndef APPREMOTE_SYNC_LAST_ID
#define APPREMOTE_SYNC_LAST_ID 7U
#endif

#ifndef APPREMOTE_SYNC_MAX_TARGETS
#define APPREMOTE_SYNC_MAX_TARGETS 8U
#endif

#ifndef APPREMOTE_SYNC_QUERY_TIMEOUT_MS
#define APPREMOTE_SYNC_QUERY_TIMEOUT_MS 200U
#endif

#ifndef APPREMOTE_SYNC_PING_TIMEOUT_MS
#define APPREMOTE_SYNC_PING_TIMEOUT_MS 30U
#endif

#ifndef APPREMOTE_SYNC_PING_RETRY
#define APPREMOTE_SYNC_PING_RETRY 3U
#endif

#ifndef APPREMOTE_SYNC_PING_MIN_OK
#define APPREMOTE_SYNC_PING_MIN_OK 2U
#endif

#ifndef APPREMOTE_SYNC_PING_GAP_MS
#define APPREMOTE_SYNC_PING_GAP_MS 2U
#endif

#ifndef APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS
#define APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS 30U
#endif

#ifndef APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS
#define APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS 30U
#endif

#ifndef APPREMOTE_SYNC_DELAY_AFTER_ARM_MS
#define APPREMOTE_SYNC_DELAY_AFTER_ARM_MS 30U
#endif

#ifndef APPREMOTE_SYNC_DELAY_AFTER_RELEASE_MS
#define APPREMOTE_SYNC_DELAY_AFTER_RELEASE_MS 30U
#endif

#ifndef APPREMOTE_SYNC_CMD_RETRY
#define APPREMOTE_SYNC_CMD_RETRY 3U
#endif

#ifndef APPREMOTE_SYNC_CMD_RETRY_GAP_MS
#define APPREMOTE_SYNC_CMD_RETRY_GAP_MS 5U
#endif

typedef struct {
    uint8_t current_id;
} app_remote_state_t;

static app_remote_state_t s;

/* 受信キュー */
static volatile uint8_t s_q_head = 0U;
static volatile uint8_t s_q_tail = 0U;
static char s_q_lines[APPREMOTE_Q_SZ][APPREMOTE_LINE_MAX];

/* 同期対象 */
static uint8_t s_sync_target_ids[APPREMOTE_SYNC_MAX_TARGETS];
static uint8_t s_sync_target_count = 0U;
static uint8_t s_last_sync_ok[APPREMOTE_NODE_MAX] = {0U};

typedef enum {
    APPREMOTE_SYNC_FAIL_NONE = 0,
    APPREMOTE_SYNC_FAIL_SCAN,
    APPREMOTE_SYNC_FAIL_ARM_SEND,
    APPREMOTE_SYNC_FAIL_ARM_STAT,
    APPREMOTE_SYNC_FAIL_MCLK_OFF_CMD,
    APPREMOTE_SYNC_FAIL_MCLK_OFF_WAIT,
    APPREMOTE_SYNC_FAIL_PRIME,
    APPREMOTE_SYNC_FAIL_PRIME_OFF_WAIT,
    APPREMOTE_SYNC_FAIL_RELEASE_SEND,
    APPREMOTE_SYNC_FAIL_RELEASE_STAT,
    APPREMOTE_SYNC_FAIL_MCLK_ON_CMD,
    APPREMOTE_SYNC_FAIL_MCLK_ON_WAIT,
    APPREMOTE_SYNC_FAIL_VERIFY,
} AppRemoteSyncFailReason;

static volatile AppRemoteSyncFailReason s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_NONE;
volatile uint8_t g_appremote_sync_fail_reason = 0U;
volatile uint8_t g_appremote_sync_fail_id = 0xFFU;

volatile uint8_t g_appremote_sync_target_count = 0U;
volatile uint8_t g_appremote_sync_target_ids[8] = {
    0xFFU, 0xFFU, 0xFFU, 0xFFU,
    0xFFU, 0xFFU, 0xFFU, 0xFFU
};

/* private helpers */
static void appremote_queue_push(const char* line);
static bool appremote_queue_pop(char* out, size_t out_sz);
static void appremote_rs485_callback(const char* line);

static const char* skip_separators(const char* p);

static void clear_last_sync_ok(void);
static void sync_targets_clear(void);
static bool sync_targets_add(uint8_t id);

static bool query_sync_targets(uint8_t expect_ready,
                               uint8_t require_clean,
                               uint32_t to_ms);
static bool verify_sync_targets(void);
static bool remote_sync_arm_targets(void);
static bool remote_sync_release_targets(void);
static bool wait_master_mclk(uint8_t expect_mclk,
                             uint32_t total_timeout_ms);

/* --------------------------------------------------------------------------
 * 受信キュー
 * -------------------------------------------------------------------------- */

static void appremote_queue_push(const char* line)
{
    if (!line || !*line) {
        return;
    }

    const size_t n = strnlen(line, APPREMOTE_LINE_MAX - 1U);

    __disable_irq();

    const uint8_t next = (uint8_t)((s_q_head + 1U) % APPREMOTE_Q_SZ);

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
    if (!out || out_sz == 0U) {
        return false;
    }

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

static void appremote_rs485_callback(const char* line)
{
    appremote_queue_push(line);
}

/* --------------------------------------------------------------------------
 * 共通ヘルパ
 * -------------------------------------------------------------------------- */

static const char* skip_separators(const char* p)
{
    while (*p == ' ' || *p == ':') {
        ++p;
    }

    return p;
}

/* --------------------------------------------------------------------------
 * 同期対象管理
 * -------------------------------------------------------------------------- */

static void clear_last_sync_ok(void)
{
    for (uint8_t i = 0U; i < APPREMOTE_NODE_MAX; ++i) {
        s_last_sync_ok[i] = 0U;
    }
}

static void sync_targets_clear(void)
{
    s_sync_target_count = 0U;
}

static bool sync_targets_add(uint8_t id)
{
    id &= 0x0FU;

    if (id >= APPREMOTE_NODE_MAX) {
        return false;
    }

    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        if (s_sync_target_ids[i] == id) {
            return true;  /* 重複登録は成功扱い */
        }
    }

    if (s_sync_target_count >= APPREMOTE_SYNC_MAX_TARGETS) {
        return false;
    }

    s_sync_target_ids[s_sync_target_count++] = id;
    return true;
}

static bool query_sync_targets(uint8_t expect_ready,
                               uint8_t require_clean,
                               uint32_t to_ms)
{
    bool all_ok = true;

    if (s_sync_target_count == 0U) {
        return false;
    }

    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        const uint8_t id = s_sync_target_ids[i];
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

static bool verify_sync_targets(void)
{
    bool all_ok = true;

    if (s_sync_target_count == 0U) {
        return false;
    }

    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        const uint8_t id = s_sync_target_ids[i];
        RemoteSyncStat st;

        if (remote_query_sync_stat_to(id, &st, APPREMOTE_SYNC_QUERY_TIMEOUT_MS) &&
            st.ready == 1U &&
            st.dirty == 0U) {
            s_last_sync_ok[id] = 1U;
        } else {
            s_last_sync_ok[id] = 0U;
            all_ok = false;
        }
    }

    return all_ok;
}

static bool remote_sync_arm_targets(void)
{
    bool all_ok = true;

    if (s_sync_target_count == 0U) {
        return false;
    }

    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        const uint8_t id = s_sync_target_ids[i];
        bool ok = false;

        for (uint8_t retry = 0U; retry < APPREMOTE_SYNC_CMD_RETRY; ++retry) {
            if (remote_sync_arm_to(id)) {
                ok = true;
                break;
            }

            HAL_Delay(APPREMOTE_SYNC_CMD_RETRY_GAP_MS);
        }

        if (!ok) {
            s_last_sync_ok[id] = 0U;
            g_appremote_sync_fail_id = id;
            all_ok = false;
        }
    }

    return all_ok;
}

static bool remote_sync_release_targets(void)
{
    bool all_ok = true;

    if (s_sync_target_count == 0U) {
        return false;
    }

    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        const uint8_t id = s_sync_target_ids[i];

        if (!remote_sync_release_to(id)) {
            s_last_sync_ok[id] = 0U;
            all_ok = false;
        }
    }

    return all_ok;
}

static bool wait_master_mclk(uint8_t expect_mclk,
                             uint32_t total_timeout_ms)
{
    const uint32_t start = HAL_GetTick();

    do {
        RemoteSyncStat st0;

        if (remote_query_sync_stat_to(0U, &st0, APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
            if (st0.mclk == expect_mclk) {
                return true;
            }
        }

        HAL_Delay(20U);
    } while ((uint32_t)(HAL_GetTick() - start) < total_timeout_ms);

    return false;
}

/* --------------------------------------------------------------------------
 * public: 初期化 / 対象ID
 * -------------------------------------------------------------------------- */

void AppRemote_Init(void)
{
    s.current_id = 0U;
    s_q_head = 0U;
    s_q_tail = 0U;

    RS485_Bridge_Init();
    RS485_RegisterUICallback(appremote_rs485_callback);
}

void AppRemote_SetID(uint8_t id)
{
    s.current_id = (uint8_t)(id & 0x0FU);
}

uint8_t AppRemote_GetID(void)
{
    return s.current_id;
}

/* --------------------------------------------------------------------------
 * public: 受信行処理
 * -------------------------------------------------------------------------- */

bool AppRemote_ParseLine(const char* line, AppRemote_Event* ev)
{
    if (!line || !ev) {
        return false;
    }

    char buf[APPREMOTE_LINE_MAX];
    size_t len = strlen(line);

    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1U;
    }

    for (size_t i = 0U; i < len; ++i) {
        buf[i] = (char)tolower((unsigned char)line[i]);
    }
    buf[len] = '\0';

    ev->type = APPREMOTE_EVT_NONE;
    ev->id = 0xFFU;
    ev->value = 0U;

    const char* p = buf;

    if ((p[0] == '#' || p[0] == '@') && isxdigit((unsigned char)p[1])) {
        ev->id = (uint8_t)strtoul(&p[1], NULL, 16) & 0x0FU;
        p = skip_separators(p + 2);
    }

    if (strcmp(p, "run") == 0) {
        ev->type = APPREMOTE_EVT_RUN;
        return true;
    }

    if (strcmp(p, "stop") == 0) {
        ev->type = APPREMOTE_EVT_STOP;
        return true;
    }

    if (strncmp(p, "stat:volt", 9U) == 0) {
        p = skip_separators(p + 9U);
        ev->type = APPREMOTE_EVT_STAT_VOLT;
        ev->value = (uint32_t)(atoi(p) / 1000);
        return true;
    }

    if (strncmp(p, "stat:phas", 9U) == 0) {
        p = skip_separators(p + 9U);
        ev->type = APPREMOTE_EVT_STAT_PHAS;
        ev->value = (uint32_t)atoi(p);
        return true;
    }

    if (strncmp(p, "stat:tcurr", 10U) == 0) {
        p = skip_separators(p + 10U);
        ev->type = APPREMOTE_EVT_STAT_TRIP_CURR;
        ev->value = (uint32_t)atoi(p);
        return true;
    }

    if (strncmp(p, "stat:freq", 9U) == 0) {
        p = skip_separators(p + 9U);
        ev->type = APPREMOTE_EVT_STAT_FREQ;
        ev->value = (uint32_t)strtoul(p, NULL, 10);
        return true;
    }

    if (strncmp(p, "stat:ready", 10U) == 0) {
        p = skip_separators(p + 10U);
        ev->type = APPREMOTE_EVT_STAT_READY;
        ev->value = (uint32_t)strtoul(p, NULL, 10);
        return true;
    }

    if (strncmp(p, "stat:mclk", 9U) == 0) {
        p = skip_separators(p + 9U);
        ev->type = APPREMOTE_EVT_STAT_MCLK;
        ev->value = (uint32_t)strtoul(p, NULL, 10);
        return true;
    }

    return false;
}

void AppRemote_HandleLine(const char* line)
{
    appremote_queue_push(line);
}

bool AppRemote_PopLine(char* out, size_t out_sz)
{
    return appremote_queue_pop(out, out_sz);
}

void AppRemote_Process(void)
{
    RS485_Bridge_Poll();
}

/* --------------------------------------------------------------------------
 * public: 基本制御 / 設定 / 計測
 * -------------------------------------------------------------------------- */

bool AppRemote_Run(void)
{
    return remote_run_to(s.current_id);
}

bool AppRemote_Stop(void)
{
    return remote_stop_to(s.current_id);
}

bool AppRemote_QueryState(void)
{
    return remote_request_state_report_to(s.current_id, 600U);
}

bool AppRemote_SetVolt(uint32_t mv)
{
    return remote_set_pot_volt_to(s.current_id, mv);
}

bool AppRemote_SetTripCurr(uint32_t ma)
{
    return remote_set_trip_curr_to(s.current_id, ma);
}

bool AppRemote_SetPhas(uint16_t deg)
{
    return remote_set_phase_to(s.current_id, deg);
}

bool AppRemote_MeasVolt(int32_t* mv, uint32_t to_ms)
{
    return remote_meas_volt_mV_to(s.current_id, mv, to_ms);
}

bool AppRemote_MeasCurr(int32_t* ma, uint32_t to_ms)
{
    return remote_meas_curr_mA_to(s.current_id, ma, to_ms);
}

/* --------------------------------------------------------------------------
 * public: 同期制御
 * -------------------------------------------------------------------------- */

bool AppRemote_ScanSyncTargets(uint32_t timeout_ms)
{
    char reply[16];
    uint8_t hit_count[APPREMOTE_NODE_MAX] = {0U};

    sync_targets_clear();

    for (uint8_t pass = 0U; pass < APPREMOTE_SYNC_PING_RETRY; ++pass) {
        for (uint8_t id = APPREMOTE_SYNC_FIRST_ID;
             id <= APPREMOTE_SYNC_LAST_ID && id < APPREMOTE_NODE_MAX;
             ++id) {

            if (remote_ping_to_timeout(id, reply, sizeof(reply), timeout_ms)) {
                hit_count[id]++;
            }

            HAL_Delay(APPREMOTE_SYNC_PING_GAP_MS);
        }
    }

    for (uint8_t id = APPREMOTE_SYNC_FIRST_ID;
         id <= APPREMOTE_SYNC_LAST_ID && id < APPREMOTE_NODE_MAX;
         ++id) {

        if (hit_count[id] >= APPREMOTE_SYNC_PING_MIN_OK) {
            sync_targets_add(id);
        }
    }


    g_appremote_sync_target_count = s_sync_target_count;

    for (uint8_t i = 0U; i < 8U; ++i) {
        if (i < s_sync_target_count) {
            g_appremote_sync_target_ids[i] = s_sync_target_ids[i];
        } else {
            g_appremote_sync_target_ids[i] = 0xFFU;
        }
    }

    /* 同期運転では CH0 がクロックマスタ。CH0 不在なら同期不可。 */
    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        if (s_sync_target_ids[i] == 0U) {
            return true;
        }
    }

    return false;
}

uint8_t AppRemote_GetSyncTargetCount(void)
{
    return s_sync_target_count;
}

uint8_t AppRemote_GetSyncTargetId(uint8_t index)
{
    if (index >= s_sync_target_count) {
        return 0xFFU;
    }

    return s_sync_target_ids[index];
}

bool AppRemote_GetLastSyncOk(uint8_t id)
{
    if (id >= APPREMOTE_NODE_MAX) {
        return false;
    }

    return s_last_sync_ok[id] != 0U;
}

uint8_t AppRemote_GetLastSyncFailReason(void)
{
    return (uint8_t)s_last_sync_fail_reason;
}

/*
 * 本番同期シーケンス:
 *
 * 1) 起動中ノードを scan する
 * 2) 対象ノードへ SYNC:ARM を個別送信
 *    - AD9833 は Reset保持
 *    - freq/phase ロード済み
 * 3) master MCLK OFF を確認
 * 4) Reset保持中に MCLK PRIME
 *    - 初回 MCLK gate ON を本番前に空打ちする
 * 5) MCLK OFF を再確認
 * 6) 対象ノードへ DDS:RELEASE を個別送信
 * 7) READY=1, DIRTY=0 を確認
 * 8) guard wait 後、master MCLK ON
 *
 * 同期開始点は最後の MCLK ON。
 */
bool AppRemote_SyncStart(void)
{
    clear_last_sync_ok();
    s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_NONE;
    g_appremote_sync_fail_reason = 0U;
    g_appremote_sync_fail_id = 0xFFU;

    if (!AppRemote_ScanSyncTargets(APPREMOTE_SYNC_PING_TIMEOUT_MS)) {
        s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_SCAN;
        g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    /*
     * ARM:
     * MCLK が必要。
     * 各ノードで AD9833 に周波数・位相をロードし、Reset保持で待機させる。
     */
    if (!remote_sync_arm_targets()) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_ARM_SEND;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }
    HAL_Delay(APPREMOTE_SYNC_DELAY_AFTER_ARM_MS);

    /*
     * ARM完了確認。
     * READY=0: まだ Reset解除前
     * DIRTY=0: 周波数・位相ロード完了
     */
    if (!query_sync_targets(0U, 1U, APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_ARM_STAT;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    /*
     * DDS設定が終わった後で MCLK OFF。
     * ここから位相アキュムレータを進ませない。
     */
    if (!AppRemote_SyncStopMaster()) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_MCLK_OFF_CMD;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    /* DDS:RELEASE に進む前に、ch0 の MCLK gate が実際に OFF になったことを確認する。 */
    if (!wait_master_mclk(0U, 1000U)) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_MCLK_OFF_WAIT;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    /*
     * Reset保持中に MCLK gate を一度空打ちする。
     *
     * 目的:
     *   初回の 5PB1102 / MCLK 経路の立ち上がりを
     *   本番同期開始前に消費する。
     *
     * 前提:
     *   この時点では SYNC:ARM 済みなので、各DDSは Reset保持中。
     *   PRIME 中に MCLK が入っても位相アキュムレータは進まない。
     */
    if (!remote_sync_prime_master()) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_PRIME;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    if (!wait_master_mclk(0U, 1000U)) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_PRIME_OFF_WAIT;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    HAL_Delay(APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS);

    /* RELEASE: Reset解除。MCLKは不要。scan で見つかった同期対象だけへ個別送信する。 */
    if (!remote_sync_release_targets()) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_RELEASE_SEND;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }
    HAL_Delay(APPREMOTE_SYNC_DELAY_AFTER_RELEASE_MS);

    /*
     * RELEASE完了確認。
     * READY=1: Reset解除済み、開始待ち
     * DIRTY=0: DDS設定は最新
     */
    if (!query_sync_targets(1U, 1U, APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_RELEASE_STAT;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    /*
     * 全chが Reset解除済み READY=1 になった後、すぐ MCLK ON しない。
     * この期間、MCLKはOFFなので位相アキュムレータは進まない。
     */
    HAL_Delay(APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS);

    /* MCLK ON: ここが全chの同期開始点。 */
    if (!AppRemote_SyncGoMaster()) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_MCLK_ON_CMD;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    if (!wait_master_mclk(1U, 1000U)) {
    	s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_MCLK_ON_WAIT;
    	g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    if (!verify_sync_targets()) {
        s_last_sync_fail_reason = APPREMOTE_SYNC_FAIL_VERIFY;
        g_appremote_sync_fail_reason = (uint8_t)s_last_sync_fail_reason;
        return false;
    }

    return true;

}

bool AppRemote_SyncGoMaster(void)
{
    return remote_sync_go_master();
}

bool AppRemote_SyncStopMaster(void)
{
    return remote_sync_stop_master();
}

bool AppRemote_QuerySyncStat(RemoteSyncStat* st, uint32_t to_ms)
{
    return remote_query_sync_stat_to(s.current_id, st, to_ms);
}
