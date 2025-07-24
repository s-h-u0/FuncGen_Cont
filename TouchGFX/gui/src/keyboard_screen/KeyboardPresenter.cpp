#include <gui/keyboard_screen/KeyboardPresenter.hpp>
#include <gui/keyboard_screen/KeyboardView.hpp>

KeyboardPresenter::KeyboardPresenter(KeyboardView& v) : view(v) {}

void KeyboardPresenter::onDigit(uint8_t d)
{
    const uint32_t nv = currentValue * 10 + d;
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
    if (model) {
        model->setDesiredValue(currentValue);
        model->setLastInputValue(currentValue);  // ← これを追加
    }
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

void KeyboardPresenter::activate()
{
    if (model) {
        currentValue = model->getLastInputValue();  // 復元
    }
}
