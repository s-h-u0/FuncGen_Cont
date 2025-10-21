#include <gui/model/Model.hpp>
#include "rs485_bridge.h"
#include <string.h>
#include <stdio.h>

// ===== 追加: 受信キュー =====
#ifndef REM_LINE_MAX
#define REM_LINE_MAX  64
#endif
#ifndef REM_Q_SZ
#define REM_Q_SZ      8
#endif

static volatile uint8_t remHead = 0, remTail = 0;
static char remLines[REM_Q_SZ][REM_LINE_MAX];

// g_currentID の実体をここで定義（UI全体で共有される）
uint8_t g_currentID = 0;

static Model* s_model_singleton = nullptr;

namespace {
inline uint8_t idx(SettingType t) {
    switch(t) {
        case SettingType::Voltage: return 0;
        case SettingType::Phase:   return 1;
        default:                   return 0;
    }
}
}



extern "C" void RS485_UI_Trampoline(const char* line)
{
    if (s_model_singleton) s_model_singleton->handleRemoteLine(line);
}


Model::Model() : modelListener(nullptr) {
	s_model_singleton = this;
	RS485_RegisterUICallback(RS485_UI_Trampoline);
    for (int i = 0; i < MAX_ID; i++) {
        desiredVoltages[i]   = 0;
        desiredPhases[i]     = 0;
        lastInputVoltages[i] = 0;
        lastInputPhases[i]   = 0;
    }
    currentSetting = SettingType::Voltage;
}

void Model::bind(ModelListener* listener) { modelListener = listener; }
void Model::triggerSomeEvent() { if (modelListener) modelListener->onSomeEvent(); }

void Model::setDesiredValue(SettingType t, uint32_t v, uint8_t id) {
    if (id >= MAX_ID) return;
    if (t == SettingType::Voltage) desiredVoltages[id] = v;
    else if (t == SettingType::Phase) desiredPhases[id] = v;
}

uint32_t Model::getDesiredValue(SettingType t, uint8_t id) const {
    if (id >= MAX_ID) return 0;
    if (t == SettingType::Voltage) return desiredVoltages[id];
    else if (t == SettingType::Phase) return desiredPhases[id];
    return 0;
}

void Model::setLastInputValue(SettingType t, uint32_t v, uint8_t id) {
    if (id >= MAX_ID) return;
    if (t == SettingType::Voltage) lastInputVoltages[id] = v;
    else if (t == SettingType::Phase) lastInputPhases[id] = v;
}

uint32_t Model::getLastInputValue(SettingType t, uint8_t id) const {
    if (id >= MAX_ID) return 0;
    if (t == SettingType::Voltage) return lastInputVoltages[id];
    else if (t == SettingType::Phase) return lastInputPhases[id];
    return 0;
}

void Model::setCurrentSetting(SettingType s) { currentSetting = s; }
SettingType Model::getCurrentSetting() const { return currentSetting; }


// ★ 差し替え: ここは“即Presenter呼び出し”をやめて、キューに積むだけ
void Model::handleRemoteLine(const char* line)
{
    if (!line || !*line) return;
    size_t n = strnlen(line, REM_LINE_MAX - 1);

    // 短いクリティカルセクション（競合回避）
    __disable_irq();
    uint8_t next = (uint8_t)((remHead + 1) % REM_Q_SZ);
    if (next != remTail) {                    // フルでなければ積む
        memcpy(remLines[remHead], line, n);
        remLines[remHead][n] = '\0';
        remHead = next;
    }
    // フルの場合は“捨てる”ポリシー（必要なら上書きに変えてもOK）
    __enable_irq();
}

// ★ 差し替え: tick() でキューを吐く（= GUIスレッドで Presenter を呼ぶ）
void Model::tick()
{
    for (;;) {
        char line[REM_LINE_MAX];

        __disable_irq();
        if (remTail == remHead) { __enable_irq(); break; } // 空なら終了
        strcpy(line, remLines[remTail]);
        remTail = (uint8_t)((remTail + 1) % REM_Q_SZ);
        __enable_irq();

        if (modelListener) modelListener->onRemoteLine(line);
    }
}
