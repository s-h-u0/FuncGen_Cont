// Model.cpp
#include <gui/model/Model.hpp>

namespace {
inline uint8_t idx(SettingType t) {
    return (t == SettingType::Voltage) ? 0 : 1;
}
}

Model::Model() : modelListener(nullptr) {}

void Model::bind(ModelListener* listener)
{
    modelListener = listener;
}

void Model::tick() {}

void Model::triggerSomeEvent()
{
    if (modelListener) {
        modelListener->onSomeEvent();
    }
}

void Model::setDesiredValue(SettingType t, uint32_t v)
{
    desiredValues[idx(t)] = v;
}

uint32_t Model::getDesiredValue(SettingType t) const
{
    return desiredValues[idx(t)];
}

void Model::setLastInputValue(SettingType t, uint32_t v)
{
    lastInputValues[idx(t)] = v;
}

uint32_t Model::getLastInputValue(SettingType t) const
{
    return lastInputValues[idx(t)];
}

void Model::setCurrentSetting(SettingType s)
{
    currentSetting = s;
}

SettingType Model::getCurrentSetting() const
{
    return currentSetting;
}
