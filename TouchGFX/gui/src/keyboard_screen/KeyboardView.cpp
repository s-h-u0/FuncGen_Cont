#include <gui/keyboard_screen/KeyboardView.hpp>
#include <touchgfx/Unicode.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/SettingType.hpp>
#include <texts/TextKeysAndLanguages.hpp>


KeyboardView::KeyboardView() : KeyboardViewBase() {}

void KeyboardView::setupScreen()
{
    KeyboardViewBase::setupScreen();
    if (presenter) {
        presenter->activate();
        updateDisplay();                     // 数値表示
        updateUnit(presenter->getCurrentSetting()); // 単位表示
    }
}


void KeyboardView::tearDownScreen()
{
    KeyboardViewBase::tearDownScreen();
}

void KeyboardView::updateDisplay()
{
    if (!presenter) return;

    uint32_t v = presenter->getCurrentValue();
    Unicode::snprintf(
        Setting_ValueBuffer,
        SETTING_VALUE_SIZE,
        "%u",
        static_cast<unsigned>(v)
    );
    Setting_Value.invalidate();

    // ★ ここで単位更新
    updateUnit(presenter->getCurrentSetting());
}

void KeyboardView::gotoMainScreen()
{
    application().gotoMainScreenNoTransition();
}

void KeyboardView::setLabelAccordingToSetting(SettingType setting)
{
    if (setting == SettingType::Voltage) {
        Setting_Label.setTypedText(touchgfx::TypedText(T_VOLTAGE));
        // Unit_Label.setTypedText(touchgfx::TypedText(T_UNIT_VOLT)); // 例: 単位ラベルがあるなら
    } else {
        Setting_Label.setTypedText(touchgfx::TypedText(T_PHASE));
        // Unit_Label.setTypedText(touchgfx::TypedText(T_UNIT_DEG));  // 例
    }
    Setting_Label.invalidate();
    // Unit_Label.invalidate();
}


void KeyboardView::updateUnit(SettingType s)
{
    // 1) 直接文字列で入れる方法
    //if (s == SettingType::Voltage) {
    //    // ASCII なので snprintf でOK
    //    Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, "%s", "V");
    //} else {
    //    // 「°」はUnicode直接代入する
    //    static const touchgfx::Unicode::UnicodeChar DEGREE_STR[] = { 0x00B0, 0 }; // 0x00B0 = '°'
    //    touchgfx::Unicode::strncpy(textArea1Buffer, DEGREE_STR, TEXTAREA1_SIZE);
    //}
    //textArea1.invalidate();

    // 2) texts.xlsx に T_UNIT_V / T_UNIT_DEG など登録済みならこちらでもOK
    textArea1.setTypedText(touchgfx::TypedText(
        s == SettingType::Voltage ? T_UNIT_V : T_UNIT_DEG
    ));
    textArea1.invalidate();
}

// 数字キー
void KeyboardView::One_()    { if (presenter) presenter->onDigit(1); }
void KeyboardView::Two_()    { if (presenter) presenter->onDigit(2); }
void KeyboardView::Three_()  { if (presenter) presenter->onDigit(3); }
void KeyboardView::Four_()   { if (presenter) presenter->onDigit(4); }
void KeyboardView::Five_()   { if (presenter) presenter->onDigit(5); }
void KeyboardView::Six_()    { if (presenter) presenter->onDigit(6); }
void KeyboardView::Seven_()  { if (presenter) presenter->onDigit(7); }
void KeyboardView::Eight_()  { if (presenter) presenter->onDigit(8); }
void KeyboardView::Nine_()   { if (presenter) presenter->onDigit(9); }
void KeyboardView::Zero_()   { if (presenter) presenter->onDigit(0); }

void KeyboardView::Delete_() { if (presenter) presenter->onDelete(); }
void KeyboardView::Enter_()  { if (presenter) presenter->onEnter(); }
