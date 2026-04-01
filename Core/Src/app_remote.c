/*
 * app_remote.c
 *
 *  Created on: Mar 31, 2026
 *      Author: shu-morishima
 */


#include "app_remote.h"
#include "remote_client.h"
#include "rs485_bridge.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef APPREMOTE_LINE_MAX
#define APPREMOTE_LINE_MAX 64
#endif

#ifndef APPREMOTE_Q_SZ
#define APPREMOTE_Q_SZ 8
#endif

typedef struct {
    uint8_t current_id;
    bool running;
} app_remote_state_t;

static app_remote_state_t s;

/* ===== 受信キュー ===== */
static volatile uint8_t s_q_head = 0;
static volatile uint8_t s_q_tail = 0;
static char s_q_lines[APPREMOTE_Q_SZ][APPREMOTE_LINE_MAX];

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
    s.running = false;

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

bool AppRemote_Run(void)
{
    if (remote_run_to(s.current_id)) {
        s.running = true;
        return true;
    }
    return false;
}

bool AppRemote_Stop(void)
{
    if (remote_stop_to(s.current_id)) {
        s.running = false;
        return true;
    }
    return false;
}

bool AppRemote_QueryState(void)
{
    return remote_query_state_to(s.current_id);
}

bool AppRemote_MeasVolt(int32_t* mv, uint32_t to_ms)
{
    return remote_meas_volt_mV_to(s.current_id, mv, to_ms);
}

bool AppRemote_SetVolt(uint32_t mv)
{
    return remote_set_pot_volt_to(s.current_id, mv);
}

bool AppRemote_SetPhas(uint16_t deg)
{
    return remote_set_phas_to(s.current_id, deg);
}

bool AppRemote_IsRunning(void)
{
    return s.running;
}

void AppRemote_SetRunning(bool running)
{
    s.running = running;
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

    return false;
}


void AppRemote_Process(void)
{
    RS485_Bridge_Poll();
}
