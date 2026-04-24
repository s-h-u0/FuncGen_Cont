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
 *      * TripCurrent は子機の TRIP:CURR、つまりトリップ電流設定を表す
 *  - スレッドモデル: TouchGFX の単一スレッド想定（排他制御なし）
 */
class Model
{
public:
    static constexpr uint8_t kMaxId = 16;

    Model();
    void bind(ModelListener* listener);
    void pushRemoteLine(const char* line);
    void tick();
    void triggerSomeEvent();

    void setDesiredValue(SettingType t, uint32_t v, uint8_t id);
    uint32_t getDesiredValue(SettingType t, uint8_t id) const;

    void setLastInputValue(SettingType t, uint32_t v, uint8_t id);
    uint32_t getLastInputValue(SettingType t, uint8_t id) const;

    void setCurrentSetting(SettingType s);
    SettingType getCurrentSetting() const;

    void noteAlive(uint8_t id, uint32_t now_ms);
    bool isLikelyAlive(uint8_t id, uint32_t now_ms, uint32_t alive_ms = 1500) const;

    void setRunning(uint8_t id, bool r);
    bool isRunning(uint8_t id) const;

    void setSynced(uint8_t id, bool v);
    bool isSynced(uint8_t id) const;

    void setDesiredFreq(uint8_t id, uint32_t hz);
    uint32_t getDesiredFreq(uint8_t id) const;

    void setDdsArmed(uint8_t id, bool v);
    bool isDdsArmed(uint8_t id) const;

    void setMclkEnabled(uint8_t id, bool v);
    bool isMclkEnabled(uint8_t id) const;

    void setSyncNeeded(uint8_t id, bool v);
    bool isSyncNeeded(uint8_t id) const;

private:
    ModelListener* modelListener;

    uint32_t desiredVoltages[kMaxId];
    uint32_t desiredTripCurrents[kMaxId];
    uint32_t desiredPhases[kMaxId];

    uint32_t lastInputVoltages[kMaxId];
    uint32_t lastInputTripCurrents[kMaxId];
    uint32_t lastInputPhases[kMaxId];

    uint32_t lastSeenTick[kMaxId];

    uint32_t desiredFreqs[kMaxId];
    bool     ddsArmed[kMaxId];
    bool     mclkEnabled[kMaxId];
    bool     synced[kMaxId];
    bool     running[kMaxId];
    bool     syncNeeded[kMaxId];

    SettingType currentSetting {SettingType::Voltage};
};
