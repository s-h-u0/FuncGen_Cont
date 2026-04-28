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

#ifndef APPREMOTE_SYNC_MAX_TARGETS
#define APPREMOTE_SYNC_MAX_TARGETS 8U
#endif

#ifndef APPREMOTE_SYNC_PING_RETRY
#define APPREMOTE_SYNC_PING_RETRY      3U
#endif

#ifndef APPREMOTE_SYNC_PING_MIN_OK
#define APPREMOTE_SYNC_PING_MIN_OK     1U
#endif

#ifndef APPREMOTE_SYNC_PING_GAP_MS
#define APPREMOTE_SYNC_PING_GAP_MS     2U
#endif

#ifndef APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS
#define APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS   500U
#endif

#ifndef APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS
#define APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS    500U
#endif

static uint8_t s_sync_target_ids[APPREMOTE_SYNC_MAX_TARGETS];
static uint8_t s_sync_target_count = 0U;

static uint8_t s_last_sync_ok[8] = {0};

/* forward declarations */
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

static void sync_targets_clear(void)
{
    s_sync_target_count = 0U;
}



#ifndef APPREMOTE_SYNC_PING_TIMEOUT_MS
#define APPREMOTE_SYNC_PING_TIMEOUT_MS 30U
#endif

bool AppRemote_ScanSyncTargets(uint32_t timeout_ms)
{
    char reply[16];

    sync_targets_clear();

    uint8_t hit_count[8] = {0U};

    for (uint8_t pass = 0U; pass < APPREMOTE_SYNC_PING_RETRY; ++pass) {
        for (uint8_t id = APPREMOTE_SYNC_FIRST_ID;
             id <= APPREMOTE_SYNC_LAST_ID && id < 8U;
             ++id) {

            if (remote_ping_to_timeout(id, reply, sizeof(reply), timeout_ms)) {
                hit_count[id]++;
            }

            HAL_Delay(APPREMOTE_SYNC_PING_GAP_MS);
        }
    }

    for (uint8_t id = APPREMOTE_SYNC_FIRST_ID;
         id <= APPREMOTE_SYNC_LAST_ID && id < 8U;
         ++id) {

        if (hit_count[id] >= APPREMOTE_SYNC_PING_MIN_OK) {
            sync_targets_add(id);
        }
    }
    /*
     * 同期運転では CH0 がクロックマスタなので、
     * CH0 が見つからない場合は同期不可。
     */
    for (uint8_t i = 0U; i < s_sync_target_count; ++i) {
        if (s_sync_target_ids[i] == 0U) {
            return true;
        }
    }

    return false;
}
static bool sync_targets_add(uint8_t id)
{
    id &= 0x0FU;

    if (id >= 8U) {
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

        if (!remote_sync_arm_to(id)) {
            s_last_sync_ok[id] = 0U;
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



static void clear_last_sync_ok(void)
{
    for (uint8_t i = 0; i < 8; ++i) {
        s_last_sync_ok[i] = 0;
    }
}


/* DEBUG ONLY: 手動切り分け用。製品のSyncボタンからは呼ばない。 */
bool AppRemote_SyncStart_Test_0_3(void)
{
    RemoteSyncStat st;

    clear_last_sync_ok();

    /*
     * 手動で成功した構成に固定:
     * ch0 = master
     * ch3 = slave
     */
    sync_targets_clear();

    if (!sync_targets_add(0U)) return false;
    if (!sync_targets_add(3U)) return false;

    /*
     * ARM
     */
    if (!remote_sync_arm_to(0U)) return false;
    HAL_Delay(300);

    if (!remote_sync_arm_to(3U)) return false;
    HAL_Delay(300);

    /*
     * ARM確認
     */
    if (!remote_query_sync_stat_to(0U, &st, 300U)) return false;
    if (st.ready != 0U || st.dirty != 0U) return false;

    if (!remote_query_sync_stat_to(3U, &st, 300U)) return false;
    if (st.ready != 0U || st.dirty != 0U) return false;

    /*
     * MCLK OFF
     */
    if (!remote_sync_stop_master()) return false;
    HAL_Delay(300);

    if (!remote_query_sync_stat_to(0U, &st, 300U)) return false;
    if (st.mclk != 0U) return false;

    /*
     * 追加:
     * Reset保持中に MCLK gate を一度空打ちする。
     * 初回 Gate ON の不安定さをここで消費する。
     */
    if (!remote_sync_prime_master()) return false;

    if (!remote_query_sync_stat_to(0U, &st, 300U)) return false;
    if (st.mclk != 0U) return false;

    HAL_Delay(APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS);

    /*
     * RELEASE
     */
    if (!remote_sync_release_to(0U)) return false;
    HAL_Delay(300);

    if (!remote_sync_release_to(3U)) return false;
    HAL_Delay(300);


    /*
     * RELEASE確認
     */
    if (!remote_query_sync_stat_to(0U, &st, 300U)) return false;
    if (st.ready != 1U || st.dirty != 0U) return false;

    if (!remote_query_sync_stat_to(3U, &st, 300U)) return false;
    if (st.ready != 1U || st.dirty != 0U) return false;
    /*
     * RELEASE 後、MCLK ON 前の guard。
     * この間 MCLK は OFF なので、位相アキュムレータは進まない。
     */
    HAL_Delay(APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS);

    /*
     * MCLK ON
     */
    if (!remote_sync_go_master()) return false;

    if (!remote_query_sync_stat_to(0U, &st, 300U)) return false;
    if (st.mclk != 1U) return false;

    s_last_sync_ok[0] = 1U;
    s_last_sync_ok[3] = 1U;

    return true;
}




bool AppRemote_GetLastSyncOk(uint8_t id)
{
    if (id >= 8) return false;
    return s_last_sync_ok[id] != 0;
}



bool AppRemote_SyncStart(void)
{
    clear_last_sync_ok();

    if (!AppRemote_ScanSyncTargets(APPREMOTE_SYNC_PING_TIMEOUT_MS)) {
        return false;
    }

    /*
     * ARM:
     * MCLK が必要。
     * 各ノードで AD9833 に周波数・位相をロードし、Reset保持で待機させる。
     */
    if (!remote_sync_arm_targets()) {
        return false;
    }
    HAL_Delay(300);

    /*
     * ARM完了確認。
     * READY=0: まだ Reset解除前
     * DIRTY=0: 周波数・位相ロード完了
     */
    if (!query_sync_targets(0U, 1U, APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
        return false;
    }

    /*
     * DDS設定が終わった後で MCLK OFF。
     * ここから位相アキュムレータを進ませない。
     */
    if (!AppRemote_SyncStopMaster()) {
        return false;
    }

    /*
     * DDS:RELEASE に進む前に、ch0 の MCLK gate が
     * 実際に OFF になったことを確認する。
     */
    if (!wait_master_mclk(0U, 1000U)) {
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
     *   この時点では SYNC:ARM 済みなので、
     *   各DDSは Reset保持中。
     *   したがって PRIME 中に MCLK が入っても
     *   位相アキュムレータは進まない。
     */
    if (!remote_sync_prime_master()) {
        return false;
    }

    /*
     * PRIME 後に、MCLK が OFF に戻っていることを再確認する。
     */
    if (!wait_master_mclk(0U, 1000U)) {
        return false;
    }

    /*
     * MCLK OFF 確認後の物理安定待ち。
     */
    HAL_Delay(APPREMOTE_SYNC_GUARD_AFTER_MCLK_OFF_MS);


    /*
     * RELEASE:
     * Reset解除。MCLKは不要。
     * scan で見つかった同期対象だけへ個別送信する。
     */
    if (!remote_sync_release_targets()) {
        return false;
    }
    HAL_Delay(300);

    /*
     * RELEASE完了確認。
     * READY=1: Reset解除済み、開始待ち
     * DIRTY=0: DDS設定は最新
     */
    if (!query_sync_targets(1U, 1U, APPREMOTE_SYNC_QUERY_TIMEOUT_MS)) {
        return false;
    }

    /*
     * 重要:
     * 全chが Reset解除済み READY=1 になったあと、
     * すぐ MCLK ON しない。
     * この期間、MCLKはOFFなので位相アキュムレータは進まない。
     */
    HAL_Delay(APPREMOTE_SYNC_GUARD_AFTER_RELEASE_MS);

    /*
     * MCLK ON:
     * ここが全chの同期開始点。
     */
    if (!AppRemote_SyncGoMaster()) {
        return false;
    }

    /*
     * ch0 の MCLK gate が実際に ON になったことを確認する。
     */
    if (!wait_master_mclk(1U, 1000U)) {
        return false;
    }

    return verify_sync_targets();
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

        HAL_Delay(20);

    } while ((uint32_t)(HAL_GetTick() - start) < total_timeout_ms);

    return false;
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

