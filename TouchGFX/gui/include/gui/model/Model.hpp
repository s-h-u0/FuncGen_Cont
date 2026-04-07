// Model.hpp
#pragma once
#include <cstdint>
#include <gui/common/SettingType.hpp>
#include <gui/model/ModelListener.hpp>

/**
 * @file   Model.hpp
 * @brief  アプリのドメイン状態（設定値・編集中の項目）を保持する層
 * @details
 *  - 役割: View/Presenter とデバイス層の間で「設定値」を一元管理
 *  - 不変条件:
 *      * 設定値キャッシュと入力履歴は ID ごとに保持する
 *      * desired* は子機から最後に確認した設定値キャッシュを表す
 *      * lastInput* は親機UIが最後に入力した値の履歴を表す
 *      * currentSetting は常に有効な SettingType
 *  - スレッドモデル: TouchGFX の単一スレッド想定（排他制御なし）
 */
class Model
{
public:

	static constexpr uint8_t kMaxId = 16;

    /** @brief 既定値（0）で初期化 */
    Model();

    /**
     * @brief  Presenter 側のリスナをバインド
     * @param  listener Model からの通知を受ける先（Presenter）
     * @note   ライフサイクルの早い段階で1回呼ばれる想定
     */
    void bind(ModelListener* listener);

    void pushRemoteLine(const char* line);

    void tick();


    /**
     * @brief  サンプル用イベントを発火（ModelListener へ通知）
     * @note   onSomeEvent() をオーバーライドして使う
     */
    void triggerSomeEvent();

    // ★ 新API：種類を指定して set/get

    /**
     * @brief 子機から最後に確認した設定値キャッシュを保持する
     * @details
     *  - 子機が正本であり、この値は親機が確認済みの設定値を表す
     *  - 送信要求直後の未確認値を表すものではない
     *  - 確定更新は主に MainPresenter::onRemoteLine() の状態反映で行う
     */
    void setDesiredValue(SettingType t, uint32_t v, uint8_t id);
    uint32_t getDesiredValue(SettingType t, uint8_t id) const;

    /**
     * @brief 親機UIが最後に入力した値を入力履歴として保持する
     * @details
     *  - キーボード画面などでユーザーが入力した値の履歴を表す
     *  - 子機の正本状態そのものではない
     *  - 再編集時の初期値や、UI文脈の保持に使う
     */
    void setLastInputValue(SettingType t, uint32_t v, uint8_t id);
    uint32_t getLastInputValue(SettingType t, uint8_t id) const;


    /**
     * @brief  現在編集中の設定項目を切替
     * @param  s SettingType
     */
    void setCurrentSetting(SettingType s);

    /**
     * @brief  現在編集中の設定項目を取得
     * @return SettingType（未設定時は既定の Voltage）
     */
    SettingType getCurrentSetting() const;



    static constexpr uint8_t MAX_ID = 16;

    /** @brief 子機からの応答を最後に確認した時刻を管理する */
    void noteAlive(uint8_t id, uint32_t now_ms);
    bool isLikelyAlive(uint8_t id, uint32_t now_ms, uint32_t alive_ms = 1500) const;



    void setRunning(uint8_t id, bool r);
    bool isRunning(uint8_t id) const;

    /**
     * @brief 親機キャッシュがこの時点で子機確認済みかを表す
     * @details
     *  - true は、現在の親機キャッシュが子機応答で確認済みであることを表す
     *  - false は、要求送信後の確認待ちだけでなく、画面表示直後などの再確認前も含む
     */
    void setSynced(uint8_t id, bool v);
    bool isSynced(uint8_t id) const;

    // Model.hpp
    void setDesiredFreq(uint8_t id, uint32_t hz);
    uint32_t getDesiredFreq(uint8_t id) const;

    void setDdsArmed(uint8_t id, bool v);
    bool isDdsArmed(uint8_t id) const;

    void setMclkEnabled(uint8_t id, bool v);
    bool isMclkEnabled(uint8_t id) const;



private:
    /** @brief Presenter へ通知するためのリスナ */
    ModelListener* modelListener;


    uint32_t desiredVoltages[MAX_ID];
    uint32_t desiredCurrents[MAX_ID];
    uint32_t desiredPhases[MAX_ID];

    uint32_t lastInputVoltages[MAX_ID];
    uint32_t lastInputCurrents[MAX_ID];
    uint32_t lastInputPhases[MAX_ID];

    uint32_t lastSeenTick[MAX_ID];

    // Model のメンバ
    uint32_t desiredFreqs[kMaxId];
    bool     ddsArmed[kMaxId];
    bool     mclkEnabled[kMaxId];

    /** @brief 親機キャッシュが現在の子機状態と同期済みかを表すフラグ */
    bool synced[MAX_ID];

    /** @brief 子機から最後に確認した RUN/STOP 状態キャッシュ */
    bool running[MAX_ID];



    /** @brief 現在編集中の設定項目（既定: Voltage） */
    SettingType currentSetting {SettingType::Voltage};
};
