#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ===== 明示的にIDを渡す新API（本命）===== */
bool remote_ping_to(uint8_t id, char* out, size_t out_sz);
bool remote_run_to(uint8_t id);
bool remote_stop_to(uint8_t id);
bool remote_set_freq_to(uint8_t id, uint32_t hz);
bool remote_set_func_to(uint8_t id, const char* name);
bool remote_set_phas_to(uint8_t id, uint16_t deg);
bool remote_set_pot_volt_to(uint8_t id, uint32_t volt);
bool remote_set_curr_to(uint8_t id, uint32_t ma);
bool remote_meas_volt_mV_to(uint8_t id, int32_t* mv, uint32_t timeout_ms);
bool remote_meas_curr_mV_to(uint8_t id, int32_t* mv, uint32_t timeout_ms);
bool remote_query_state_to(uint8_t id);




#ifdef __cplusplus
}
#endif

#endif /* REMOTE_CLIENT_H */
