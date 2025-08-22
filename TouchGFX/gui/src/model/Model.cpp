// Model.cpp
#include <gui/model/Model.hpp>

/**
 * @brief SettingType を配列添字（0:Voltage, 1:Phase）に写像
 * @note  2種前提の軽量化ヘルパ。将来項目を増やす場合は switch へ変更推奨。
 */
namespace {
inline uint8_t idx(SettingType t) {
    return (t == SettingType::Voltage) ? 0 : 1;
}
}

Model::Model() : modelListener(nullptr) {
    desiredValues[0] = 0; // Voltage
    desiredValues[1] = 0; // Phase
    lastInputValues[0] = 0;
    lastInputValues[1] = 0;
    currentSetting = SettingType::Voltage;
}
void Model::bind(ModelListener* listener)
{
    modelListener = listener;
}

void Model::tick() {}  // 現状は処理なし（将来の周期処理用フック）

void Model::triggerSomeEvent()
{
    if (modelListener) {
        modelListener->onSomeEvent();
    }
}

void Model::setDesiredValue(SettingType t, uint32_t v)
{
    // 必要ならここで範囲チェック/クランプを行う
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
