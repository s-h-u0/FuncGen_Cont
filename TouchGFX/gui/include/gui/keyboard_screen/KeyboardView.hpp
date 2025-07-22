#pragma once

#include <gui_generated/keyboard_screen/KeyboardViewBase.hpp>
#include <gui/keyboard_screen/KeyboardPresenter.hpp>

class KeyboardView : public KeyboardViewBase
{
public:
    KeyboardView();
    virtual ~KeyboardView() {}

    virtual void setupScreen();
    virtual void tearDownScreen();

    void bindPresenter(KeyboardPresenter* p);

    void updateDisplay(); // 入力値を表示するための関数

private:
    KeyboardPresenter* presenter {nullptr};
};
