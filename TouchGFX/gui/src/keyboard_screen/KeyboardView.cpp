/**
 * @file   KeyboardView.cpp
 * @brief  キーボード画面の View 実装（TouchGFX）
 * @details
 *  - 役割: 画面要素の表示更新／ボタン入力の受け取り → Presenter へ通知
 *  - 不変条件:
 *      * 各キー入力は 120ms のデバウンスを通過した場合のみ Presenter へ伝える
 *      * 単位（V/°）やラベルは SettingType に応じて切り替える
 *  - 生成物の取り扱い:
 *      * *KeyboardViewBase* は TouchGFX Designer の自動生成。本ファイルで拡張
 *  - スレッド: TouchGFX 単一スレッド前提（排他制御なし）
 */

#include <gui/keyboard_screen/KeyboardView.hpp>
#include <touchgfx/Unicode.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <gui/common/SettingType.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include "main.h"              // HAL_GPIO, HAL_GetTick() などを定義したヘッダ
#include "stm32f4xx_hal.h"     // HAL_GetTick() を使うならこちらだけでも可




/** @brief コンストラクタ（重い処理はしない） */
KeyboardView::KeyboardView() : KeyboardViewBase() {}

/**
 * @brief 画面初期化：Presenter を起動し表示/単位を同期
 * @details
 *  1) Presenter::activate() を呼び、Model の状態を View に反映する準備をさせる
 *  2) 数値表示（updateDisplay）と単位表示（updateUnit）を現在の SettingType で更新
 */
void KeyboardView::setupScreen()
{
    KeyboardViewBase::setupScreen();
    if (presenter) {
        presenter->activate();
        updateDisplay();                           // 数値表示
        updateUnit(presenter->getCurrentSetting()); // 単位表示
    }
}

/** @brief 画面終了処理（現状は Base へ委譲のみ） */
void KeyboardView::tearDownScreen()
{
    KeyboardViewBase::tearDownScreen();
}

/**
 * @brief Presenter の編集中値を画面へ反映（数値フォーマットは整数）
 * @note  単位は @ref updateUnit で SettingType に応じて都度更新
 */
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

    // ★ ここで単位更新（数値と単位のズレ防止）
    updateUnit(presenter->getCurrentSetting());
}

/**
 * @brief メイン画面へ遷移（Presenter から呼び出されるラッパ）
 * @note  遷移アニメなし（NoTransition）
 */
void KeyboardView::gotoMainScreen()
{
    application().gotoMainScreenNoTransition();
}

/**
 * @brief SettingType に応じて見出しラベルを切り替え
 * @param setting Voltage / Phase
 * @note  texts.xlsx 側に T_VOLTAGE / T_PHASE を登録しておくこと
 */
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

/**
 * @brief 単位ラベル（V/°）を SettingType に応じて更新
 * @param s Voltage なら "V"、Phase なら "°"
 * @details
 *  - 直接文字列で代入する方法と、texts.xlsx のキーで切り替える方法がある。
 *  - 下の実装は texts.xlsx のキー（T_UNIT_V / T_UNIT_DEG）を利用。
 */
void KeyboardView::updateUnit(SettingType s)
{
    // 1) 直接文字列で入れる方法（サンプル）
    // if (s == SettingType::Voltage) {
    //     Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, "%s", "V"); // ASCII は snprintf でOK
    // } else {
    //     // 「°」は Unicode 直接代入
    //     static const touchgfx::Unicode::UnicodeChar DEGREE_STR[] = { 0x00B0, 0 }; // 0x00B0 = '°'
    //     touchgfx::Unicode::strncpy(textArea1Buffer, DEGREE_STR, TEXTAREA1_SIZE);
    // }
    // textArea1.invalidate();

    // 2) texts.xlsx に T_UNIT_V / T_UNIT_DEG など登録済みならこちらでもOK
    UNIT.setTypedText(touchgfx::TypedText(
        s == SettingType::Voltage ? T_UNIT_V : T_UNIT_DEG
    ));
    UNIT.invalidate();
}

/**
 * @brief デバウンス判定（120ms、wrap-around 安全）
 * @param id 対象キー
 * @return true=押下を許可 / false=弾く
 * @details
 *  - HAL_GetTick() は約49日で周回するため、(now - last) を int32_t で評価して安全に比較。
 *  - 最終押下 tick はキーごと（配列）に保持する。
 */
bool KeyboardView::allowPress(KeyId id)
{
    uint32_t now = HAL_GetTick();
    uint32_t& last = keyLastTick[static_cast<uint8_t>(id)];
    if ((int32_t)(now - last) < (int32_t)kDebounceMs) return false; // wrap-around安全
    last = now;
    return true;
}

/**
 * @brief 数字キーの共通ハンドラ（Presenter->onDigit へ転送）
 * @param d  0..9
 * @param id デバウンス用キーID
 * @note  presenter 未接続時やデバウンスNG時は何もしない
 */
void KeyboardView::pressDigit(uint8_t d, KeyId id)
{
    if (!presenter) return;
    if (!allowPress(id)) return;
    presenter->onDigit(d);
}

/**
 * @brief アクションキーの共通ハンドラ（Presenter のメンバ関数を呼ぶ）
 * @param fn  呼び出す Presenter メンバ関数（onDelete / onEnter）
 * @param id  デバウンス用キーID
 * @note  presenter 未接続時やデバウンスNG時は何もしない
 */
void KeyboardView::pressAction(void (KeyboardPresenter::*fn)(), KeyId id)
{
    if (!presenter) return;
    if (!allowPress(id)) return;
    (presenter->*fn)();
}

void KeyboardView::setInitialValue(uint32_t v)
{
    // Presenter 側の currentValue にも反映されている想定だが、
    // 念のため直接画面の数値も更新
    Unicode::snprintf(Setting_ValueBuffer, SETTING_VALUE_SIZE, "%u",
                      static_cast<unsigned>(v));
    Setting_Value.invalidate();
}

/* =========================
 *  数字キー → 共通ヘルパへ
 * ========================= */
void KeyboardView::Zero_()  { pressDigit(0, KeyId::K0); }
void KeyboardView::One_()   { pressDigit(1, KeyId::K1); }
void KeyboardView::Two_()   { pressDigit(2, KeyId::K2); }
void KeyboardView::Three_() { pressDigit(3, KeyId::K3); }
void KeyboardView::Four_()  { pressDigit(4, KeyId::K4); }
void KeyboardView::Five_()  { pressDigit(5, KeyId::K5); }
void KeyboardView::Six_()   { pressDigit(6, KeyId::K6); }
void KeyboardView::Seven_() { pressDigit(7, KeyId::K7); }
void KeyboardView::Eight_() { pressDigit(8, KeyId::K8); }
void KeyboardView::Nine_()  { pressDigit(9, KeyId::K9); }

/* =========================
 *  アクションキー → 共通ヘルパへ
 * ========================= */
void KeyboardView::Delete_() { pressAction(&KeyboardPresenter::onDelete, KeyId::KDel); }
void KeyboardView::Enter_()  { pressAction(&KeyboardPresenter::onEnter , KeyId::KEnter); }
