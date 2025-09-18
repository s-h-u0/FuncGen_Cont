#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>   // ← size_t が必要なら追加

#ifdef __cplusplus
extern "C" {
#endif

// --- 一致させる ---
bool remote_ping(char* out, size_t out_sz);   // 引数ありに統一
bool remote_run(void);
bool remote_stop(void);
bool remote_set_freq(uint32_t hz);
bool remote_set_func(const char* name);       // const char* に統一
bool remote_set_phas(uint16_t deg);           // .c 側と同じ uint16_t
bool remote_set_pot_volt(uint32_t volt);
bool remote_meas_volt_mV(int32_t* mv);        // int32_t* に統一

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_CLIENT_H */
