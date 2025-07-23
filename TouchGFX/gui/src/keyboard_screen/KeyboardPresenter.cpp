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
        model->setDesiredValue(currentValue);   // Model に保存
    }
    view.gotoMainScreen();                      // 画面遷移（View 経由）
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
