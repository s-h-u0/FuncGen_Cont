#include <gui/model/Model.hpp>
#include "app_remote.h"

Model::Model() : modelListener(nullptr)
{
    for (int i = 0; i < MAX_ID; i++) {
        desiredVoltages[i]   = 0;
        desiredCurrents[i]   = 0;
        desiredPhases[i]     = 0;

        lastInputVoltages[i] = 0;
        lastInputCurrents[i] = 0;
        lastInputPhases[i]   = 0;

        lastSeenTick[i]      = 0;
        running[i]           = false;
        synced[i] = false;
        desiredFreqs[i] = 0;
        ddsArmed[i] = false;
        mclkEnabled[i] = false;
    }
    currentSetting = SettingType::Voltage;
}

void Model::bind(ModelListener* listener)
{
    modelListener = listener;
}

void Model::triggerSomeEvent()
{
    if (modelListener) {
        modelListener->onSomeEvent();
    }
}

void Model::setDesiredValue(SettingType t, uint32_t v, uint8_t id)
{
    if (id >= MAX_ID) return;

    if (t == SettingType::Voltage) {
        desiredVoltages[id] = v;
    } else if (t == SettingType::Current) {
        desiredCurrents[id] = v;
    } else if (t == SettingType::Phase) {
        desiredPhases[id] = v;
    }
}

uint32_t Model::getDesiredValue(SettingType t, uint8_t id) const
{
    if (id >= MAX_ID) return 0;

    if (t == SettingType::Voltage) return desiredVoltages[id];
    if (t == SettingType::Current) return desiredCurrents[id];
    if (t == SettingType::Phase)   return desiredPhases[id];
    return 0;
}

void Model::setLastInputValue(SettingType t, uint32_t v, uint8_t id)
{
    if (id >= MAX_ID) return;

    if (t == SettingType::Voltage) {
        lastInputVoltages[id] = v;
    } else if (t == SettingType::Current) {
        lastInputCurrents[id] = v;
    } else if (t == SettingType::Phase) {
        lastInputPhases[id] = v;
    }
}

uint32_t Model::getLastInputValue(SettingType t, uint8_t id) const
{
    if (id >= MAX_ID) return 0;

    if (t == SettingType::Voltage) return lastInputVoltages[id];
    if (t == SettingType::Current) return lastInputCurrents[id];
    if (t == SettingType::Phase)   return lastInputPhases[id];
    return 0;
}

void Model::setCurrentSetting(SettingType s)
{
    currentSetting = s;
}

SettingType Model::getCurrentSetting() const
{
    return currentSetting;
}

void Model::tick()
{
    char line[64];

    while (AppRemote_PopLine(line, sizeof(line))) {
        pushRemoteLine(line);
    }
}

void Model::pushRemoteLine(const char* line)
{
    if (modelListener && line) {
        modelListener->onRemoteLine(line);
    }
}

void Model::noteAlive(uint8_t id, uint32_t now_ms)
{
    if (id < MAX_ID) {
        lastSeenTick[id] = now_ms;
    }
}

bool Model::isLikelyAlive(uint8_t id, uint32_t now_ms, uint32_t alive_ms) const
{
    if (id >= MAX_ID) return false;

    const uint32_t t = lastSeenTick[id];
    if (!t) return false;

    return (int32_t)(now_ms - t) < (int32_t)alive_ms;
}

void Model::setRunning(uint8_t id, bool r)
{
    if (id < MAX_ID) {
        running[id] = r;
    }
}

bool Model::isRunning(uint8_t id) const
{
    if (id >= MAX_ID) return false;
    return running[id];
}

void Model::setSynced(uint8_t id, bool v)
{
    if (id < MAX_ID) {
        synced[id] = v;
    }
}

bool Model::isSynced(uint8_t id) const
{
    if (id >= MAX_ID) return false;
    return synced[id];
}

void Model::setDesiredFreq(uint8_t id, uint32_t hz)
{
    if (id < MAX_ID) {
        desiredFreqs[id] = hz;
    }
}

uint32_t Model::getDesiredFreq(uint8_t id) const
{
    if (id >= MAX_ID) return 0;
    return desiredFreqs[id];
}

void Model::setDdsArmed(uint8_t id, bool v)
{
    if (id < MAX_ID) {
        ddsArmed[id] = v;
    }
}

bool Model::isDdsArmed(uint8_t id) const
{
    if (id >= MAX_ID) return false;
    return ddsArmed[id];
}

void Model::setMclkEnabled(uint8_t id, bool v)
{
    if (id < MAX_ID) {
        mclkEnabled[id] = v;
    }
}

bool Model::isMclkEnabled(uint8_t id) const
{
    if (id >= MAX_ID) return false;
    return mclkEnabled[id];
}
