#pragma once
#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>
#include <gui/common/SettingType.hpp>

class KeyboardView;

class KeyboardPresenter : public touchgfx::Presenter,
                          public ModelListener
{
public:
    explicit KeyboardPresenter(KeyboardView& v);
    void activate() override;

    void onDigit(uint8_t d);
    void onDelete();
    void onEnter();
    void reset();

    SettingType getCurrentSetting() const;


    uint32_t getCurrentValue() const;

private:
    KeyboardView& view;
    uint32_t currentValue{0};
    static constexpr uint32_t MAX_INPUT = 9999;
};
