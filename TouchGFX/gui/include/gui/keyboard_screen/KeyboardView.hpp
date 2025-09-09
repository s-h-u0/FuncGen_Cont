#pragma once
/**
 * @file   KeyboardView.hpp
 * @brief  キーボード画面の表示・入力（TouchGFX View 層）
 * @details
 *  - 役割: ボタン入力を受け取り Presenter に伝達、Presenter 指示で数値/単位ラベルを更新する。
 *  - 不変条件:
 *      * 各キーは 120ms のデバウンスを通過したときのみ Presenter へ通知される。
 *      * 単位（V / °）やラベルは SettingType に応じて切り替わる。
 *  - 生成物の取り扱い:
 *      * *KeyboardViewBase* は TouchGFX Designer の自動生成。編集せず、本クラスで拡張する。
 *  - スレッドモデル: TouchGFX の単一スレッド前提（排他制御は不要）。
 */

class KeyboardPresenter;   // 前方宣言（循環依存回避）

#include <gui_generated/keyboard_screen/KeyboardViewBase.hpp>
#include <gui/common/SettingType.hpp>

/**
 * @class  KeyboardView
 * @brief  キーボード画面の View 実装（入力→Presenter、表示更新）
 * @note   Base で宣言された各ボタンのコールバックを本クラスで実装する。
 */
class KeyboardView : public KeyboardViewBase
{
public:
    /** @brief コンストラクタ（重い処理はしない） */
	KeyboardView();

    /** @brief 画面初期化：Presenter を起動し表示/単位を同期 */
    void setupScreen()    override;

    /** @brief 画面終了処理（現状は Base へ委譲のみ） */
    void tearDownScreen() override;

    /** @brief 数値表示を Presenter の編集中値に合わせて更新 */
    void updateDisplay();

    /** @brief Presenter から呼ばれるメイン画面遷移のラッパ */
    void gotoMainScreen();

    /** @brief SettingType に応じて見出しラベルを切り替え（Voltage/Phase） */
    void setLabelAccordingToSetting(SettingType setting);

    /** @brief 単位ラベル（V/°）を SettingType に応じて更新 */
    void updateUnit(SettingType s);

    /* 個別ボタンコールバック（Base に virtual 定義あり）
     * @warning TouchGFX Designer と結線されているため、シグネチャ/名前は変更しないこと。
     */
    void One_()    override;  ///< 数字キー「1」
    void Two_()    override;  ///< 数字キー「2」
    void Three_()  override;  ///< 数字キー「3」
    void Four_()   override;  ///< 数字キー「4」
    void Five_()   override;  ///< 数字キー「5」
    void Six_()    override;  ///< 数字キー「6」
    void Seven_()  override;  ///< 数字キー「7」
    void Eight_()  override;  ///< 数字キー「8」
    void Nine_()   override;  ///< 数字キー「9」
    void Zero_()   override;  ///< 数字キー「0」
    void Delete_() override;  ///< 1桁削除
    void Enter_()  override;  ///< 入力確定（Presenter 経由で保存→画面遷移）

    void setInitialValue(uint32_t v);

private:
    /** @brief 1キーの最小再押下間隔[ms]（誤連打抑止のためのデバウンス） */
    static constexpr uint32_t kDebounceMs = 120;

    /** @brief キー識別子（デバウンス用の配列添字に使用） */
    enum class KeyId : uint8_t {
        K0, K1, K2, K3, K4, K5, K6, K7, K8, K9, KDel, KEnter, Count
    };

    /** @brief 各キーの最終押下 Tick（HAL_GetTick 準拠の値） */
    uint32_t keyLastTick[static_cast<uint8_t>(KeyId::Count)] = {0};

    /**
     * @brief デバウンス判定（wrap-around 安全な差分比較）
     * @param id 対象キー
     * @return true=押下許可 / false=弾く
     * @note  実装は .cpp 側（HAL_GetTick に依存させるためヘッダへは出さない）。
     */
    bool allowPress(KeyId id);

    /**
     * @brief 数字キーの共通ハンドラ（Presenter->onDigit へ転送）
     * @param d 0..9
     * @param id デバウンス用キーID
     * @note  実装は .cpp 側。機能は変えずに重複をまとめている。
     */
    void pressDigit(uint8_t d, KeyId id);

    /**
     * @brief アクションキーの共通ハンドラ（Presenter のメンバ関数を呼ぶ）
     * @param fn  呼び出す Presenter メンバ関数ポインタ（onDelete/onEnter）
     * @param id  デバウンス用キーID
     * @note  実装は .cpp 側。HAL 依存をヘッダに持ち込まないための分離。
     */
    void pressAction(void (KeyboardPresenter::*fn)(), KeyId id);
};
