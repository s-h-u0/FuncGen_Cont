#pragma once
/**
 * @file   KeyboardPresenter.hpp
 * @brief  キーボード画面の Presenter（View と Model の仲介）
 * @details
 *  - 役割: キー入力に応じて編集中の数値を更新し、View に描画更新を指示。
 *           入力確定時には Model に保存し、画面遷移をトリガーする。
 *  - 不変条件:
 *      * currentValue は常に 0..MAX_INPUT の範囲内。
 *      * getCurrentSetting() は常に有効な SettingType を返す（Model 依存）。
 *  - スレッドモデル: TouchGFX の単一スレッド前提（排他制御なし）。
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
    /**
     * @brief  コンストラクタ：対応する View を束縛
     * @param  v キーボード画面の View
     * @note   Model への参照は ModelListener 経由で得る想定。
     */
    explicit KeyboardPresenter(KeyboardView& v);

    /**
     * @brief  画面表示の初期化シーケンス（Model → View 同期）
     * @details
     *  現在の編集対象（SettingType）と直近入力値を Model から取得し、
     *  View に初期表示させる。
     */
    void activate() override;

    /**
     * @brief  数字キー入力を1桁追加（範囲超過は無視）
     * @param  d 追加する桁（0..9）
     * @note   追加後の値が MAX_INPUT を超える場合は currentValue は変化しない。
     *         変更があれば View に再描画を指示する。
     */
    void onDigit(uint8_t d);

    /**
     * @brief  末尾の1桁を削除（/10）
     * @note   0 のときは 0 のまま。変更があれば View に再描画を指示。
     */
    void onDelete();

    /**
     * @brief  入力を確定
     * @details
     *  currentValue を Model の該当設定（desired/last 等の実装方針に従う）
     *  に保存し、View 側でメイン画面への遷移を行う。
     */
    void onEnter();

    /**
     * @brief  編集値を 0 にリセットし、View を更新
     */
    void reset();

    /**
     * @brief  現在の編集対象（電圧/位相など）を取得
     * @return SettingType
     * @note   実体は Model 側の状態に依存。
     */
    SettingType getCurrentSetting() const;

    /**
     * @brief  現在の編集中の数値を取得（View の表示用）
     * @return currentValue（0..MAX_INPUT）
     */
    uint32_t getCurrentValue() const;

private:
    /** @brief 対応する View（描画更新や画面遷移を指示するために使用） */
    KeyboardView& view;

    /** @brief 編集中の数値（常に 0..MAX_INPUT に収まる） */
    uint32_t currentValue{0};

    /** @brief 入力の上限（桁追加はこの値を超えない範囲でのみ許可） */
    static constexpr uint32_t MAX_INPUT = 9999;
};
