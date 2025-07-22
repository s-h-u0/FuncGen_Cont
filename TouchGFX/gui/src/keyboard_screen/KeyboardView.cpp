#include <gui/keyboard_screen/KeyboardView.hpp>

KeyboardView::KeyboardView()
{
}

void KeyboardView::setupScreen()
{
    KeyboardViewBase::setupScreen();
}

void KeyboardView::tearDownScreen()
{
    KeyboardViewBase::tearDownScreen();
}

void KeyboardView::bindPresenter(KeyboardPresenter* p)
{
    presenter = p;
}

void KeyboardView::updateDisplay()
{
    if (presenter) {
        uint32_t value = presenter->getCurrentValue();
        // 数値表示用 TextArea や Wildcard を使って更新
        // 例: Unicode::snprintf(textAreaBuffer, BUFFER_SIZE, "%u", value);
        //     textArea.invalidate();
    }
}
