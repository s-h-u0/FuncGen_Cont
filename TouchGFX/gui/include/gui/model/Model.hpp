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
 *      * desiredValues[2], lastInputValues[2] は Voltage/Phase の順で格納
 *      * currentSetting は常に有効な SettingType
 *  - スレッドモデル: TouchGFX の単一スレッド想定（排他制御なし）
 */
class Model
{
public:
    /** @brief 既定値（0）で初期化 */
    Model();

    /**
     * @brief  Presenter 側のリスナをバインド
     * @param  listener Model からの通知を受ける先（Presenter）
     * @note   ライフサイクルの早い段階で1回呼ばれる想定
     */
    void bind(ModelListener* listener);

    /**
     * @brief  1フレームごとの更新フック
     * @note   現状は処理なし。将来ポーリング等を追加する場合ここに実装
     */
    void tick();

    /**
     * @brief  サンプル用イベントを発火（ModelListener へ通知）
     * @note   onSomeEvent() をオーバーライドして使う
     */
    void triggerSomeEvent();

    // ★ 新API：種類を指定して set/get

    void setDesiredValue(SettingType t, uint32_t v, uint8_t id);
    uint32_t getDesiredValue(SettingType t, uint8_t id) const;

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

private:
    /** @brief Presenter へ通知するためのリスナ */
    ModelListener* modelListener;

    static constexpr int MAX_ID = 16;

    uint32_t desiredVoltages[MAX_ID]   {0};
    uint32_t desiredPhases[MAX_ID]     {0};
    uint32_t lastInputVoltages[MAX_ID] {0};
    uint32_t lastInputPhases[MAX_ID]   {0};

    /** @brief 現在編集中の設定項目（既定: Voltage） */
    SettingType currentSetting {SettingType::Voltage};
};
