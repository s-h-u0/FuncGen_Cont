/*
 * app_remote.h
 *
 *  Created on: Mar 31, 2026
 *      Author: shu-morishima
 */

#ifndef APP_REMOTE_H
#define APP_REMOTE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "remote_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APPREMOTE_EVT_NONE = 0,
    APPREMOTE_EVT_RUN,
    APPREMOTE_EVT_STOP,

    APPREMOTE_EVT_STAT_VOLT,
    APPREMOTE_EVT_STAT_TRIP_CURR,
    APPREMOTE_EVT_STAT_CURR = APPREMOTE_EVT_STAT_TRIP_CURR,  /* 互換名 */
    APPREMOTE_EVT_STAT_PHAS,
    APPREMOTE_EVT_STAT_FREQ,
    APPREMOTE_EVT_STAT_READY,
    APPREMOTE_EVT_STAT_MCLK
} AppRemote_EventType;

typedef struct {
    AppRemote_EventType type;
    uint8_t id;
    uint32_t value;
} AppRemote_Event;

/* 初期化 / 対象ID */
void    AppRemote_Init(void);
void    AppRemote_SetID(uint8_t id);
uint8_t AppRemote_GetID(void);

/* 受信行処理 */
bool AppRemote_ParseLine(const char* line, AppRemote_Event* ev);
void AppRemote_HandleLine(const char* line);
bool AppRemote_PopLine(char* out, size_t out_sz);
void AppRemote_Process(void);

/* 基本制御
 * RUN/STOP は要求送信のみ行う。
 * 親機内の RUN/STOP 状態はここでは保持・確定せず、
 * 上位層が QueryState 後の受信イベントで反映する。
 */
bool AppRemote_Run(void);
bool AppRemote_Stop(void);
bool AppRemote_QueryState(void);

/* 設定 / 計測 */
bool AppRemote_SetVolt(uint32_t mv);
bool AppRemote_SetTripCurr(uint32_t ma);
bool AppRemote_SetPhas(uint16_t deg);

bool AppRemote_MeasVolt(int32_t* mv, uint32_t to_ms);
bool AppRemote_MeasCurr(int32_t* ma, uint32_t to_ms);

/* 同期制御 */
bool    AppRemote_SyncStart(void);
bool    AppRemote_SyncGoMaster(void);
bool    AppRemote_SyncStopMaster(void);
bool    AppRemote_QuerySyncStat(RemoteSyncStat* st, uint32_t to_ms);

bool    AppRemote_ScanSyncTargets(uint32_t timeout_ms);
uint8_t AppRemote_GetSyncTargetCount(void);
uint8_t AppRemote_GetSyncTargetId(uint8_t index);
bool    AppRemote_GetLastSyncOk(uint8_t id);

uint8_t AppRemote_GetLastSyncFailReason(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_REMOTE_H */
