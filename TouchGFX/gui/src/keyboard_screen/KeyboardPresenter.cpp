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

extern "C" {
#include "app_remote.h"
}

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

    const SettingType s = model->getCurrentSetting();

    if (s == SettingType::ID) {
        // ★ ここで現在のIDをそのまま表示値にする
        currentValue = AppRemote_GetID();
    } else {
        // 電圧/位相は Model のキャッシュから
    	const uint8_t id = AppRemote_GetID();
    	currentValue = model->getDesiredValue(s, id);
    }

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
    const SettingType s = getCurrentSetting();
    if (s == SettingType::ID) {
        currentValue = d;
        view.updateDisplay();
        return;
    }

    uint32_t maxv = 0;
    switch (s) {
    case SettingType::Voltage: maxv = VOLT_MAX;  break;
    case SettingType::Current: maxv = CURR_MAX;  break;
    case SettingType::Phase:   maxv = PHASE_MAX; break;
    default:                   maxv = 0;         break;
    }

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
    clampToCurrentRange();

    const SettingType s = model->getCurrentSetting();
    const uint8_t id = AppRemote_GetID();

    model->setDesiredValue(s, currentValue, id);
    model->setLastInputValue(s, currentValue, id);

    switch (s) {
    case SettingType::ID:
        AppRemote_SetID((uint8_t)(currentValue & 0x0F));
        break;

    case SettingType::Voltage:
        AppRemote_SetVolt(currentValue);
        break;

    case SettingType::Current:
        AppRemote_SetCurr(currentValue);
        break;

    case SettingType::Phase:
        AppRemote_SetPhas((uint16_t)currentValue);
        break;

    default:
        break;
    }

    view.gotoMainScreen();
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
    const SettingType s = model ? model->getCurrentSetting() : SettingType::Voltage;

    uint32_t minv = 0;
    uint32_t maxv = 0;

    switch (s) {
    case SettingType::Voltage: minv = VOLT_MIN;  maxv = VOLT_MAX;  break;
    case SettingType::Current: minv = CURR_MIN;  maxv = CURR_MAX;  break;
    case SettingType::Phase:   minv = PHASE_MIN; maxv = PHASE_MAX; break;
    case SettingType::ID:      minv = 0;         maxv = 15;        break;
    default:                   minv = 0;         maxv = 0;         break;
    }

    if (currentValue < minv) currentValue = minv;
    if (currentValue > maxv) currentValue = maxv;
}

void KeyboardPresenter::setDesiredValue(SettingType t, uint32_t v) {
    if (model) {
    	const uint8_t id = AppRemote_GetID();
    	model->setDesiredValue(t, v, id);
    }
}



void KeyboardPresenter::onDigitForID(char c)
{
    if (!model) return;

    uint32_t idVal = 0;
    if (c >= '0' && c <= '9')      idVal = c - '0';
    else if (c >= 'A' && c <= 'F') idVal = 10 + (c - 'A');
    else return; // 想定外は無視

    currentValue = idVal;     // ← ここだけ更新
    view.updateDisplay();     // ← キーボード画面の Setting_Value を即反映
    // Model へは onEnter() まで書かない
}




