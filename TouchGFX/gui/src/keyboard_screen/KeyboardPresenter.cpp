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
    clampToCurrentRange();                         // ← 念のため範囲補正
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
    // 追加後の値が種別ごとの上限を超える場合は無視（Voltage:50, Phase:360）
    const SettingType s = getCurrentSetting();
    const uint32_t maxv = (s == SettingType::Voltage) ? VOLT_MAX : PHASE_MAX;

    const uint32_t nv = currentValue * 10u + d;
    if (nv <= maxv) {
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
    currentValue /= 10u;
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
    clampToCurrentRange();  // 念のため確定前に範囲内へ
    const SettingType s = model->getCurrentSetting();
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
    currentValue = 0u;
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

/**
 * @brief  現在の編集対象に応じて currentValue を許容範囲へ丸める
 * @details
 *  - 対象の範囲（上限含む）
 *      Voltage: [VOLT_MIN,  VOLT_MAX]  = [0, 50]
 *      Phase  : [PHASE_MIN, PHASE_MAX] = [0, 360]
 *  - Model 未バインドでも安全に動くよう、Voltage を既定として扱う（防御的実装）。
 *  - 外部要因（復元・他画面・将来の仕様変更等）で currentValue が範囲外になっても
 *    UI/ロジックが破綻しないようにするための最終ガード。
 * @post  currentValue は対象範囲に収まる（inclusive）
 * @note  呼び出し箇所：activate() / onEnter()（確定前後の整合を保証）
 * @complexity O(1)
 */
void KeyboardPresenter::clampToCurrentRange()
{
    // Model が無い場合は保守的に Voltage とみなす
    const SettingType s = model ? model->getCurrentSetting() : SettingType::Voltage;

    const uint32_t minv = (s == SettingType::Voltage) ? VOLT_MIN  : PHASE_MIN; // 現状どちらも 0
    const uint32_t maxv = (s == SettingType::Voltage) ? VOLT_MAX  : PHASE_MAX;

    // 下限・上限の inclusive クランプ
    if (currentValue < minv) currentValue = minv;   // 例: 負値が入っていた場合など
    if (currentValue > maxv) currentValue = maxv;   // 例: 51V / 361° など
}
