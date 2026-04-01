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

    void pushRemoteLine(const char* line);

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



    static constexpr uint8_t MAX_ID = 16;

    void noteAlive(uint8_t id, uint32_t now_ms);
    bool isLikelyAlive(uint8_t id, uint32_t now_ms, uint32_t alive_ms = 1500) const;

    bool running[MAX_ID];

    void setRunning(uint8_t id, bool r);
    bool isRunning(uint8_t id) const;

private:
    /** @brief Presenter へ通知するためのリスナ */
    ModelListener* modelListener;


    uint32_t desiredVoltages[MAX_ID]   {0};
    uint32_t desiredPhases[MAX_ID]     {0};
    uint32_t lastInputVoltages[MAX_ID] {0};
    uint32_t lastInputPhases[MAX_ID]   {0};

    uint32_t lastSeenTick[MAX_ID];



    /** @brief 現在編集中の設定項目（既定: Voltage） */
    SettingType currentSetting {SettingType::Voltage};
};
