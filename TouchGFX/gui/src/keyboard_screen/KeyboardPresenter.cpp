#include <gui/keyboard_screen/KeyboardPresenter.hpp>
#include <gui/keyboard_screen/KeyboardView.hpp>

KeyboardPresenter::KeyboardPresenter(KeyboardView& v)
    : view(v)
{
    view.bindPresenter(this);
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
    /* ここで確定値をモデルや別画面へ渡すなど自由に処理 */
}

void KeyboardPresenter::reset()
{
    currentValue = 0;
    view.updateDisplay();
}
