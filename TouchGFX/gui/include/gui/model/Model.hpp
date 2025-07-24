// Model.hpp
#pragma once
#include <cstdint>
#include <gui/common/SettingType.hpp>
#include <gui/model/ModelListener.hpp>

class Model
{
public:
    Model();

    void bind(ModelListener* listener);
    void tick();
    void triggerSomeEvent();

    // ★ 新API：種類を指定して set/get
    void setDesiredValue(SettingType t, uint32_t v);
    uint32_t getDesiredValue(SettingType t) const;

    void setLastInputValue(SettingType t, uint32_t v);
    uint32_t getLastInputValue(SettingType t) const;

    void setCurrentSetting(SettingType s);
    SettingType getCurrentSetting() const;

private:
    ModelListener* modelListener;

    // ★ Voltage(0), Phase(1)の2要素を格納
    uint32_t desiredValues[2]   {0, 0};
    uint32_t lastInputValues[2] {0, 0};

    SettingType currentSetting {SettingType::Voltage};
};
