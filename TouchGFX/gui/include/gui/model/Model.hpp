#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>
#include <gui/common/SettingType.hpp>

class ModelListener;


class Model
{
public:
    Model();

    void bind(ModelListener* listener);
    void tick();

    // ★ 追加：Presenter間で値を共有
    void setDesiredValue(uint32_t v);
    uint32_t getDesiredValue() const;

    // Model 側でイベントが発生したと仮定した関数
    void triggerSomeEvent();

    void setLastInputValue(uint32_t v);
    uint32_t getLastInputValue() const;

    // ★ 追加：SettingType 保持用 API
    void setCurrentSetting(SettingType s) { currentSetting = s; }
    SettingType getCurrentSetting() const { return currentSetting; }


private:
    // ▼ 追加：ユーザーが入力した値（例：AD5292 用抵抗値）
    uint32_t desiredValue = 0;
    uint32_t lastInputValue = 0;

    SettingType currentSetting = SettingType::Voltage; // デフォルト


protected:
    ModelListener* modelListener {nullptr};
};

#endif // MODEL_HPP
