#include <gui/keyboard_screen/KeyboardPresenter.hpp>
#include <gui/keyboard_screen/KeyboardView.hpp>

KeyboardPresenter::KeyboardPresenter(KeyboardView& v)
    : view(v)
{
    view.bindPresenter(this);
}

void KeyboardPresenter::activate() {}
void KeyboardPresenter::deactivate() {}

void KeyboardPresenter::onDigit(uint8_t d)
{
    constexpr uint32_t MAX_INPUT = 99'999'999;
    uint32_t nv = currentValue * 10 + d;
    if (nv <= MAX_INPUT) {
        currentValue = nv;
    }
}

void KeyboardPresenter::onDelete()
{
    currentValue /= 10;
}

void KeyboardPresenter::onEnter()
{
    // 必要に応じてModelに渡すなど
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

void KeyboardPresenter::onSomeEvent()
{
    // Modelからの通知を処理（必要に応じて）
}
