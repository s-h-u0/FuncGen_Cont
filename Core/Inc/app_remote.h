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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APPREMOTE_EVT_NONE = 0,
    APPREMOTE_EVT_RUN,
    APPREMOTE_EVT_STOP,
    APPREMOTE_EVT_STAT_VOLT,
    APPREMOTE_EVT_STAT_PHAS
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

bool AppRemote_Run(void);
bool AppRemote_Stop(void);
bool AppRemote_QueryState(void);
bool AppRemote_MeasVolt(int32_t* mv, uint32_t to_ms);

bool AppRemote_SetVolt(uint32_t mv);
bool AppRemote_SetPhas(uint16_t deg);

bool AppRemote_IsRunning(void);
void AppRemote_SetRunning(bool running);

void AppRemote_HandleLine(const char* line);
bool AppRemote_PopLine(char* out, size_t out_sz);

void AppRemote_Process(void);

#ifdef __cplusplus
}
#endif

#endif
