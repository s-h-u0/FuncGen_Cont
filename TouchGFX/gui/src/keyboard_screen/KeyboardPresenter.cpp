/**
 * @file   KeyboardPresenter.cpp
 * @brief  キーボード画面の Presenter 実装（View と Model の仲介）
 * @details
 *  - 役割:
 *      * View からの入力イベント（数字/削除/確定）を受けて編集中値を更新し、View に再描画を指示
 *      * 確定時（onEnter）に Model へ値を保存し、画面遷移（メイン画面へ戻る）をトリガ
 *  - 不変条件:
 *      * currentValue は常に 0..MAX_INPUT の範囲内
 *      * getCurrentSetting() は有効な SettingType を返す（Model 依存）
 *  - パフォーマンス:
 *      * 全処理は O(1)、動的確保なし（割込み/RTOSなしの TouchGFX 単スレッド前提）
 */

#include <gui/keyboard_screen/KeyboardPresenter.hpp>
#include <gui/keyboard_screen/KeyboardView.hpp>
#include <gui/model/Model.hpp>

/** @brief コンストラクタ：対応する View を束縛（Model は ModelListener 経由） */
KeyboardPresenter::KeyboardPresenter(KeyboardView& v) : view(v) {}

/**
 * @brief 画面表示の初期化（Model → View 同期）
 * @details
 *  - 現在の編集対象（SettingType）を Model から取得
 *  - 対象ごとの直近入力値（lastInputValue）で currentValue を初期化
 *  - ラベルと数値表示を更新
 * @note  model はアプリ起動時に bind されている想定。null の場合は何もしない。
 */
void KeyboardPresenter::activate()
{
    if (!model) return;

    SettingType s = model->getCurrentSetting();
    currentValue  = model->getLastInputValue(s);   // 種別別の last 値
    view.setLabelAccordingToSetting(s);
    view.updateDisplay();
}

/**
 * @brief 数字キー入力を 1 桁追加
 * @param d 追加する桁（0..9）
 * @details
 *  - currentValue * 10 + d を試算し、MAX_INPUT を超える場合は無視
 *  - 変化があれば View に再描画を指示
 * @invariant currentValue ∈ [0, MAX_INPUT]
 */
void KeyboardPresenter::onDigit(uint8_t d)
{
    uint32_t nv = currentValue * 10 + d;
    if (nv <= MAX_INPUT) {
        currentValue = nv;
        view.updateDisplay();
    }
}

/**
 * @brief 末尾 1 桁を削除（/10）
 * @details
 *  - 0 のときは 0 のまま
 *  - 変化があれば View に再描画を指示
 */
void KeyboardPresenter::onDelete()
{
    currentValue /= 10;
    view.updateDisplay();
}

/**
 * @brief 入力を確定
 * @details
 *  - 現在の編集対象（SettingType）を取得
 *  - currentValue を Model の desiredValue / lastInputValue に保存
 *  - View にメイン画面への遷移を依頼
 * @note  model が null の場合は何もしない（防御的プログラミング）
 */
void KeyboardPresenter::onEnter()
{
    if (!model) return;
    SettingType s = model->getCurrentSetting();
    model->setDesiredValue(s, currentValue);   // モデルに保存（確定値）
    model->setLastInputValue(s, currentValue); // 直近入力値も同期
    view.gotoMainScreen();                     // メイン画面へ戻る
}

/**
 * @brief 編集値を 0 にリセットし、View を更新
 * @post  currentValue == 0
 */
void KeyboardPresenter::reset()
{
    currentValue = 0;
    view.updateDisplay();
}

/**
 * @brief 現在の編集中の数値（View の表示用）を取得
 * @return 0..MAX_INPUT の範囲の整数
 */
uint32_t KeyboardPresenter::getCurrentValue() const
{
    return currentValue;
}

/**
 * @brief 現在の編集対象（電圧/位相など）を取得
 * @return Model があればその状態、なければ既定の Voltage
 * @note  通常は activate 以前に Model が bind 済みである前提
 */
SettingType KeyboardPresenter::getCurrentSetting() const
{
    return model ? model->getCurrentSetting() : SettingType::Voltage;
}
