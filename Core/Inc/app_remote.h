/*
 * app_remote.h
 *
 *  Created on: Mar 31, 2026
 *      Author: shu-morishima
 */

#ifndef APP_REMOTE_H
#define APP_REMOTE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
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
    APPREMOTE_EVT_STAT_CURR = APPREMOTE_EVT_STAT_TRIP_CURR,  // 互換名

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

bool AppRemote_ParseLine(const char* line, AppRemote_Event* ev);

void    AppRemote_Init(void);
void    AppRemote_SetID(uint8_t id);
uint8_t AppRemote_GetID(void);

/* RUN/STOP は要求送信のみ行う。
 * 親機内の RUN/STOP 状態はここでは保持・確定せず、
 * 上位層が QueryState 後の受信イベントで反映する。
 */
bool AppRemote_Run(void);
bool AppRemote_Stop(void);

bool AppRemote_QueryState(void);
bool AppRemote_MeasVolt(int32_t* mv, uint32_t to_ms);
bool AppRemote_MeasCurr(int32_t* ma, uint32_t to_ms);

bool AppRemote_SetVolt(uint32_t mv);
bool AppRemote_SetPhas(uint16_t deg);
/* 正式名: TRIP:CURR を設定する */
bool AppRemote_SetTripCurr(uint32_t ma);

/* 互換名: 既存 Presenter 移行まで残す */
bool AppRemote_SetCurr(uint32_t ma);


void AppRemote_HandleLine(const char* line);
bool AppRemote_PopLine(char* out, size_t out_sz);

void AppRemote_Process(void);
bool AppRemote_SyncGo(void);

bool AppRemote_SyncStart(void);
bool AppRemote_SyncArmAll(void);
bool AppRemote_SyncGoMaster(void);
bool AppRemote_SyncStopMaster(void);
bool AppRemote_SyncReleaseAll(void);
bool AppRemote_QuerySyncStat(RemoteSyncStat* st, uint32_t to_ms);

bool AppRemote_GetLastSyncOk(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif
