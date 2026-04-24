#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint8_t ready;
    uint8_t dirty;
    uint8_t mclk;
} RemoteSyncStat;

/* ===== 基本制御 ===== */
bool remote_ping_to(uint8_t id, char* out, size_t out_sz);

bool remote_run_to(uint8_t id);
bool remote_stop_to(uint8_t id);
bool remote_get_run_to(uint8_t id, uint8_t* run, uint32_t timeout_ms);

/* ===== 出力設定 ===== */
bool remote_set_freq_to(uint8_t id, uint32_t hz);
bool remote_get_freq_to(uint8_t id, uint32_t* hz, uint32_t timeout_ms);

bool remote_set_phase_to(uint8_t id, uint16_t deg);
bool remote_get_phase_to(uint8_t id, uint32_t* deg, uint32_t timeout_ms);

bool remote_set_pot_volt_to(uint8_t id, uint32_t volt);
bool remote_get_pot_volt_to(uint8_t id, uint32_t* volt, uint32_t timeout_ms);

/* ===== トリップしきい値 ===== */
bool remote_set_trip_curr_to(uint8_t id, uint32_t ma);
bool remote_get_trip_curr_to(uint8_t id, uint32_t* ma, uint32_t timeout_ms);

bool remote_set_trip_volt_to(uint8_t id, uint32_t mv);
bool remote_get_trip_volt_to(uint8_t id, uint32_t* mv, uint32_t timeout_ms);

bool remote_set_trip_temp_to(uint8_t id, int32_t deci_c);
bool remote_get_trip_temp_to(uint8_t id, int32_t* deci_c, uint32_t timeout_ms);

/* ===== 計測 ===== */
bool remote_meas_volt_mV_to(uint8_t id, int32_t* mv, uint32_t timeout_ms);
bool remote_meas_curr_mA_to(uint8_t id, int32_t* ma, uint32_t timeout_ms);

/* ===== 状態通知要求 ===== */
bool remote_request_state_report_to(uint8_t id, uint32_t timeout_ms);

/* ===== 同期制御 ===== */
bool remote_sync_go_master(void);
bool remote_sync_stop_master(void);
bool remote_sync_arm_all(void);
bool remote_sync_release_all(void);
bool remote_query_sync_stat_to(uint8_t id, RemoteSyncStat* st, uint32_t timeout_ms);

bool remote_get_addr_to(uint8_t id, uint8_t* addr, uint32_t timeout_ms);
bool remote_get_mclk_to(uint8_t id, uint8_t* on, uint32_t timeout_ms);
bool remote_get_sync_ready_to(uint8_t id, uint8_t* ready, uint32_t timeout_ms);


#ifdef __cplusplus
}
#endif

#endif /* REMOTE_CLIENT_H */
