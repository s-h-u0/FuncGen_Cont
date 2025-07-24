#include <gui/keyboard_screen/KeyboardView.hpp>
#include <touchgfx/Unicode.hpp>
#include <mvp/Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/SettingType.hpp>
#include <texts/TextKeysAndLanguages.hpp>

KeyboardView::KeyboardView() : KeyboardViewBase() {}

void KeyboardView::setupScreen()
{
    KeyboardViewBase::setupScreen();
    if (presenter) {
        presenter->activate();
        updateDisplay();   // 初期表示（0 または空）
    }
}

void KeyboardView::tearDownScreen()
{
    if (presenter) {
        presenter->deactivate();
    }
    KeyboardViewBase::tearDownScreen();
}

void KeyboardView::updateDisplay()
{
    if (!presenter) return;

    // Presenter から現在値を取得してバッファへ
    uint32_t v = presenter->getCurrentValue();
    Unicode::snprintf(
        Setting_ValueBuffer,
        sizeof(Setting_ValueBuffer) / sizeof(Setting_ValueBuffer[0]),
        "%u",
        static_cast<unsigned>(v)
    );
    Setting_Value.invalidate();             // Wildcard テキスト再描画
}

void KeyboardView::gotoMainScreen()
{
    application().gotoMainScreenNoTransition();
}

void KeyboardView::setLabelAccordingToSetting(SettingType setting)
{
    if (setting == SettingType::Voltage) {
        Setting_Label.setTypedText(touchgfx::TypedText(T_VOLTAGE));
    } else {
        Setting_Label.setTypedText(touchgfx::TypedText(T_PHASE));
    }
    Setting_Label.invalidate();  // 表示更新
}


// 数字キー
void KeyboardView::One_()    { presenter->onDigit(1); }
void KeyboardView::Two_()    { presenter->onDigit(2); }
void KeyboardView::Three_()  { presenter->onDigit(3); }
void KeyboardView::Four_()   { presenter->onDigit(4); }
void KeyboardView::Five_()   { presenter->onDigit(5); }
void KeyboardView::Six_()    { presenter->onDigit(6); }
void KeyboardView::Seven_()  { presenter->onDigit(7); }
void KeyboardView::Eight_()  { presenter->onDigit(8); }
void KeyboardView::Nine_()   { presenter->onDigit(9); }
void KeyboardView::Zero_()   { presenter->onDigit(0); }

// Delete ＆ Enter
void KeyboardView::Delete_() { presenter->onDelete(); }
void KeyboardView::Enter_()  { presenter->onEnter(); }
