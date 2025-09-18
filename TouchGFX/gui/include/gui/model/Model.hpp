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

    /**
     * @brief  設定値を保存（Voltage/Phase）
     * @param  t 設定対象（Voltage or Phase）
     * @param  v 設定値
     * @note   範囲チェックは呼び出し側で行う想定（必要ならここでクランプ）
     */
    void setDesiredValue(SettingType t, uint32_t v);

    /**
     * @brief  設定値を取得
     * @param  t 取得対象（Voltage or Phase）
     * @return 保存済みの設定値（未設定なら既定値 0）
     */
    uint32_t getDesiredValue(SettingType t) const;

    /**
     * @brief  直近の入力値（UI入力のエコーバッファ等）を保存
     * @param  t 対象（Voltage or Phase）
     * @param  v 値
     */
    void setLastInputValue(SettingType t, uint32_t v);

    /**
     * @brief  直近の入力値を取得
     * @param  t 対象（Voltage or Phase）
     * @return 保存済みの値（未設定なら既定値 0）
     */
    uint32_t getLastInputValue(SettingType t) const;

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

    /** @brief Voltage(0), Phase(1) の2要素を格納（順序は cpp の idx() と一致させる） */
    uint32_t desiredValues[3]   {0, 0, 0};

    /** @brief 直近の UI 入力値（未確定値の保持などに使用可） */
    uint32_t lastInputValues[3] {0, 0, 0};

    /** @brief 現在編集中の設定項目（既定: Voltage） */
    SettingType currentSetting {SettingType::Voltage};
};
