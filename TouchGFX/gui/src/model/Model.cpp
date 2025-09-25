#include <gui/model/Model.hpp>

namespace {
inline uint8_t idx(SettingType t) {
    switch(t) {
        case SettingType::Voltage: return 0;
        case SettingType::Phase:   return 1;
        default:                   return 0;
    }
}
}

Model::Model() : modelListener(nullptr) {
    for (int i = 0; i < MAX_ID; i++) {
        desiredVoltages[i]   = 0;
        desiredPhases[i]     = 0;
        lastInputVoltages[i] = 0;
        lastInputPhases[i]   = 0;
    }
    currentSetting = SettingType::Voltage;
}

void Model::bind(ModelListener* listener) { modelListener = listener; }
void Model::tick() {}
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
