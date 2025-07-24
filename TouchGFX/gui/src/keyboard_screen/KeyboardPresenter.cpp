// KeyboardPresenter.cpp
#include <gui/keyboard_screen/KeyboardPresenter.hpp>
#include <gui/keyboard_screen/KeyboardView.hpp>
#include <gui/model/Model.hpp>

KeyboardPresenter::KeyboardPresenter(KeyboardView& v) : view(v) {}

void KeyboardPresenter::activate()
{
    if (!model) return;

    SettingType s = model->getCurrentSetting();
    currentValue  = model->getLastInputValue(s);   // 種別別の last 値
    view.setLabelAccordingToSetting(s);
    view.updateDisplay();
}

void KeyboardPresenter::onDigit(uint8_t d)
{
    uint32_t nv = currentValue * 10 + d;
    if (nv <= MAX_INPUT) {
        currentValue = nv;
        view.updateDisplay();
    }
}

void KeyboardPresenter::onDelete()
{
    currentValue /= 10;
    view.updateDisplay();
}

void KeyboardPresenter::onEnter()
{
    if (!model) return;
    SettingType s = model->getCurrentSetting();

    model->setDesiredValue(s, currentValue);   // 種類別に保存
    model->setLastInputValue(s, currentValue);
    view.gotoMainScreen();
}

void KeyboardPresenter::reset()
{
    currentValue = 0;
    view.updateDisplay();
}

uint32_t KeyboardPresenter::getCurrentValue() const
{
    return currentValue;
}


SettingType KeyboardPresenter::getCurrentSetting() const
{
    return model ? model->getCurrentSetting() : SettingType::Voltage;
}
