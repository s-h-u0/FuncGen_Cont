#pragma once
/**
 * @file   KeyboardPresenter.hpp
 * @brief  キーボード画面の Presenter（View と Model の仲介）
 * @details
 *  - 入力上限は SettingType ごとに異なる:
 *      Voltage: 0..50[v]
 *      Current: 0..5000[mA]
 *      Phase  : 0..360[deg]
 *  - onDigit() は現在の SettingType の上限を超える入力を無視する。
 */

#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>
#include <gui/common/SettingType.hpp>

class KeyboardView;

/**
 * @class  KeyboardPresenter
 * @brief  キーボード画面の入力ロジックを司る Presenter
 * @details
 *  View からのイベント（数字/削除/確定）を受け取り、編集中値（currentValue）
 *  を更新・描画させる。確定時（onEnter）に Model へ値を保存してメイン画面へ戻る。
 */
class KeyboardPresenter : public touchgfx::Presenter,
                          public ModelListener
{
public:
    explicit KeyboardPresenter(KeyboardView& v);

    void activate() override;
    void onDigit(uint8_t d);
    void onDelete();
    void onEnter();
    void reset();

    SettingType getCurrentSetting() const;
    uint32_t getCurrentValue() const;

    void onDigitForID(char c);   ///< ID選択用 (A〜F)

private:
    KeyboardView& view;
    uint32_t currentValue{0};

    // ---- 入力範囲（上限は inclusive）----
    static constexpr uint32_t VOLT_MIN  = 0;
    static constexpr uint32_t VOLT_MAX  = 40;

    static constexpr uint32_t CURR_MIN  = 0;
    static constexpr uint32_t CURR_MAX  = 5000;

    static constexpr uint32_t PHASE_MIN = 0;
    static constexpr uint32_t PHASE_MAX = 360;

    void clampToCurrentRange();
};
